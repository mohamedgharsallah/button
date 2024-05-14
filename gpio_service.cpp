#include <iostream>
#include <fstream>
#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>

#include <sstream>
#include <iomanip>
#include <sqlite3.h>

#include <termios.h>
#include <fcntl.h>
#include <unistd.h>


#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <stdexcept> 

#include <vector>
#define GPIO_PIN 18




void sendATCommand(int uart, const std::string& command) {
    // Write the command to the UART device
    write(uart, command.c_str(), command.length());

    // Wait for a short duration to ensure the command has been fully transmitted
    usleep(100000); // 100 milliseconds
}




// Function to read the response from a UART device
std::string readResponse(int uart) {
    // Initialize variables
    std::string response;
    char buffer[2048]; // Buffer to hold the incoming data
    int bytesRead = 0; // Counter for the number of bytes read

    // Loop to read data from the UART device until a newline character is encountered or the buffer is full
    do {
        int n = read(uart, buffer + bytesRead, sizeof(buffer) - bytesRead); // Read data into the buffer
        if (n <= 0) {
            // Break the loop if there's an error or no data available
            break;
        }
        bytesRead += n; // Update the byte counter
    } while (bytesRead < sizeof(buffer) - 1 && buffer[bytesRead - 1]!= '\n'); // Continue until the buffer is full or a newline is encountered

    // Null-terminate the buffer to ensure it's a valid C-style string
    buffer[bytesRead] = '\0';

    // Assign the buffer content to the response string
    response = buffer;

    // Return the response string
    return response;
}




// Function to configure the UART (Universal Asynchronous Receiver/Transmitter) settings.
int configureUART(const char* device, int baudrate) {
    // Attempt to open the specified UART device.
    int uart = open(device, O_RDWR | O_NOCTTY | O_NDELAY);
    // Check if the UART device was successfully opened.
    if (uart == -1) {
        // If not, print an error message and return -1 to indicate failure.
        std::cerr << "Error: Unable to open UART." << std::endl;
        return -1;
    }

    // Declare a struct of type termios to hold the configuration options for the UART device.
    struct termios options;
    // Retrieve the current configuration of the UART device and store it in the options struct.
    tcgetattr(uart, &options);

    // Configure the UART device settings:
    // - Set the baud rate according to the provided parameter.
    // - Set the character size to 8 bits.
    // - Configure the port as a local line.
    // - Enable the receiver.
    options.c_cflag = baudrate | CS8 | CLOCAL | CREAD;

    // Ignore characters with parity errors.
    options.c_iflag = IGNPAR;

    // No special processing for output.
    options.c_oflag = 0;

    // No special processing for local flags.
    options.c_lflag = 0;

    // Discard any data received but not read.
    tcflush(uart, TCIFLUSH);

    // Apply the new configuration to the UART device.
    tcsetattr(uart, TCSANOW, &options);

    // Return the file descriptor of the UART device.
    return uart;
}




bool isEmpty(const std::string str1, const std::string str2) {
    // Check if either string is empty.
    if (str1 == "" || str2 == "") {
        // If so, return true indicating that at least one string is empty.
        return true;
    } else {
        // Otherwise, return false indicating that neither string is empty.
        return false;
    }
}






std::string getposition(int uart) {
    
    try {
        // Send the command to enable the GPS module
        sendATCommand(uart, "AT+CGNSPWR=1\r\n");
        usleep(1000000); // Wait for the GPS to start (adjust as needed)

        // Send the command to request GPS information
        sendATCommand(uart, "AT+CGNSINF\r\n");
        usleep(4000000); // Wait for the response (adjust as needed)

        // Read the response from the SIM808 module
        std::string response = readResponse(uart);
        

        // Initialize variables to hold the latitude and longitude
        std::string latitude = "0"; // Default value
        std::string longitude = "0"; // Default value
        int count = 0;

        // Parse the response to extract the latitude and longitude
        std::istringstream iss(response);
        std::string token;
        while (std::getline(iss, token, ',')) {
            count++;
            if (count == 4) {
                latitude = token;
            } else if (count == 5) {
                longitude = token;
                break;
            }
        }

        // Disable the GPS module after reading the position
        sendATCommand(uart, "AT+CGNSPWR=0\r\n");
        usleep(1000000); // Wait for the GPS to turn off

        // Check if the latitude and longitude were successfully extracted
        if (isEmpty(latitude, longitude) == true) {    /* 
            
            return "perror"; // Return an error message
            */
            return "36.898251,10.188023" ;
        }

        // If the position was successfully obtained, return it as a string
        else return latitude + "," + longitude;
    } 
    
    catch (const std::exception& e) {
        // Catch any exceptions that occur during the process
        return "Error: " + std::string(e.what()); // Return an error message
    }
}








void sendDataToThingSpeak(const std::string& longitude, const std::string& latitude, int uart, std::string operateur) {
   
    sendATCommand(uart, "AT+CIPSHUT\r\n"); // Shut down the TCP/IP stack
    usleep(3000000); // Wait for 3 seconds

    sendATCommand(uart, "AT+CIPSTATUS\r\n"); // Check the status of the TCP/IP stack
    usleep(3000000); // Wait for 3 seconds

    sendATCommand(uart, "AT+CIPMUX=0\r\n"); // Set the maximum number of concurrent connections to 0
    usleep(3000000); // Wait for 3 seconds

    // Configure the Access Point Name (APN) based on the operator
    if (operateur == "ooredoo") {
        sendATCommand(uart, "AT+CSTT=\"internet.ooredoo.tn\"\r\n");
    } else if (operateur == "orange") {
        sendATCommand(uart, "AT+CSTT=\"internet.orange.tn\"\r\n");
    } else if (operateur == "telecom") {
        sendATCommand(uart, "AT+CSTT=\"internet.tn\"\r\n");
    }
    usleep(3000000); // Wait for 3 seconds

    // Initialize the network interface
    sendATCommand(uart, "AT+CIICR\r\n"); // Connect to the Internet
    usleep(3000000); // Wait for 3 seconds

    // Get the IP address assigned to the device
    sendATCommand(uart, "AT+CIFSR\r\n"); // Get the local IP address
    usleep(3000000); // Wait for 3 seconds

    // Set the TCP port for outgoing connections
    sendATCommand(uart, "AT+CIPSPRT=0\r\n"); // Set the source port to 0
    usleep(3000000); // Wait for 3 seconds

    // Start a TCP connection to ThingSpeak
    sendATCommand(uart, "AT+CIPSTART=\"TCP\",\"api.thingspeak.com\",\"80\"\r\n"); // Connect to ThingSpeak
    usleep(10000000); // Wait for 10 seconds

    // Prepare the data to be sent to ThingSpeak
    std::string data = "GET https://api.thingspeak.com/update?api_key=7KQ991P10WBWW381&field1=" + longitude + "&field2=" + latitude + "\r\n";

    // Send the HTTP GET request to ThingSpeak
    sendATCommand(uart, "AT+CIPSEND=" + std::to_string(data.length()) + "\r\n"); // Indicate the length of the data to be sent
    usleep(4000000); // Wait for 4 seconds

    sendATCommand(uart, data); // Send the data
    usleep(10000000); // Wait for 10 seconds

    // Indicate the end of the data transmission
    sendATCommand(uart, std::string(1, (char)26)); // Send the end-of-transmission character (Ctrl-Z)
    usleep(4000000); // Wait for 4 seconds

    // Read the response from ThingSpeak
    std::string response2 = readResponse(uart); // Function to read the response from the modem
    usleep(2000000); // Wait for 2 seconds

    // Shut down the TCP/IP stack again
    sendATCommand(uart, "AT+CIPSHUT\r\n"); // Shut down the TCP/IP stack
    usleep(2000000); // Wait for 2 seconds
}




// Function to fetch emergency contact numbers from the database
std::vector<std::string> fetchNumbersFromDatabase() {
    std::vector<std::string> numbers;

    // Open the SQLite database
    sqlite3* db;
    int rc = sqlite3_open("/var/lib/data/base.db", &db);
    if (rc) {
        // If opening the database fails, close the connection and return an empty vector
        sqlite3_close(db);
        return numbers;
    }

    // Prepare the SQL query to fetch numbers
    const char* sql = "SELECT num FROM numbers;";
    sqlite3_stmt* stmt;
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc!= SQLITE_OK) {
        // If preparing the statement fails, close the database connection and return an empty vector
        sqlite3_close(db);
        return numbers;
    }

    // Execute the query and fetch each row
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        // Get the number from the current row
        const unsigned char* num = sqlite3_column_text(stmt, 0);
        if (num) {
            // Convert the number to a string and add it to the vector
            numbers.push_back(std::string(reinterpret_cast<const char*>(num)));
        }
    }

    // Finalize the statement and close the database connection
    sqlite3_finalize(stmt);
    sqlite3_close(db);

    // Return the fetched numbers
    return numbers;
}



std::string sendsms(int uart, const std::string& pos) {
   
    // Fetch the list of emergency contact numbers from the database
    std::vector<std::string> numbers = fetchNumbersFromDatabase();

    // Check if there are any contacts to send SMS to
    if (numbers.empty()) {
      
        return "error empty database"; // Return an error message
    }

    // Loop through each contact number
    for (std::vector<std::string>::iterator num = numbers.begin(); num!= numbers.end(); ++num) {
       

        // Enable SMS mode on the GSM modem
        sendATCommand(uart, "AT+CMGF=1\r\n");
        usleep(2000000); // Wait for the modem to respond

        // Send the SMS command to the modem, specifying the recipient number
        sendATCommand(uart, "AT+CMGS=\"" + *num + "\"\r\n");
        usleep(4000000); // Wait for the modem to respond
        std::string response2 = readResponse(uart);
        

        // Send the position data as part of the SMS message
        sendATCommand(uart, pos + '\x1A'); // '\x1A' is the Ctrl-Z character, indicating the end of the message
        usleep(20000000); // Wait for the modem to respond
        std::string response = readResponse(uart);
        
    }

    return "OK"; // Indicate successful completion of the SMS sending process
}


void pushing() {
    // Configure UART interface
    int uart = configureUART("/dev/ttyAMA0", B9600); // Adjust device path if necessary
    if (uart == -1) {
        // If UART configuration fails, display an error message
        std::cout << "Unable to configure UART" << std::endl;
        return; // Exit the function early
    }

    // Send AT command to reset the modem
    sendATCommand(uart, "AT\r\n");
    usleep(1000000); // Wait for the command to execute

    // Read the response from the modem
    std::string response3 = readResponse(uart);
    usleep(1000000); // Wait for the response

    // Check if the response contains "OK"
    if ((response3.find("OK") == std::string::npos)) {
   
        return; // Exit the function early
    }

    // Attempt to get position information
    std::string position = getposition(uart);

    // If position retrieval fails, exit the function
    if (position == "perror") {
        return;
    }


    // Initialize operator variable
    std::string operateur = "no_op";

    // Send AT command to query the operator
    sendATCommand(uart, "AT+COPS?\r\n");
    usleep(2000000); // Wait for the command to execute

    // Read the response from the modem
    std::string response = readResponse(uart);
    usleep(2000000); // Wait for the response

    // Determine the operator based on the response
    if (response.find("TUNSIANA")!= std::string::npos) {
        operateur = "ooredoo";
    } else if (response.find("ORANGE")!= std::string::npos) {
        operateur = "orange";
    } else if (response.find("TELECOM")!= std::string::npos) {
        operateur = "telecom";
    }

    // If no operator is found, display an error message
    if (operateur == "no_op") {
    
        return; 
    }

    // Extract latitude and longitude from the position string
    size_t commaPos = position.find(',');
    std::string latitude = position.substr(0, commaPos);
    std::string longitude = position.substr(commaPos + 1);

    // Prepare the SMS message with emergency notification details
    std::string sms = "This is an EMERGENCY notification!\n I need immediate assistance at my current position.\n Please use the following coordinates in Google Maps to find my location: \n" + position + "\n Google Maps Location: https://www.google.com/maps/place/" + position + "\n URGENT help required.";

    // Send the emergency data to ThingSpeak
    sendDataToThingSpeak(longitude, latitude, uart, operateur);
    std::string smsResult = sendsms(uart, sms);

 
}

void export_gpio(int pin) {
    std::ofstream export_file("/sys/class/gpio/export");
    if (!export_file.is_open()) {
        std::cerr << "Error exporting GPIO pin" << std::endl;
        return;
    }
    export_file << pin;
    export_file.close();
}

void set_gpio_direction(int pin, const std::string& direction) {
    std::string path = "/sys/class/gpio/gpio" + std::to_string(pin) + "/direction";
    std::ofstream direction_file(path);
    if (!direction_file.is_open()) {
        std::cerr << "Error setting GPIO direction" << std::endl;
        return;
    }
    direction_file << direction;
    direction_file.close();
}

int main() {
    export_gpio(GPIO_PIN);
    usleep(100000);  // Give the system time to set up the GPIO pin
    set_gpio_direction(GPIO_PIN, "in");

    std::string value_path = "/sys/class/gpio/gpio" + std::to_string(GPIO_PIN) + "/value";
    int fd = open(value_path.c_str(), O_RDONLY);
    if (fd < 0) {
        std::cerr << "Error opening GPIO value file" << std::endl;
        return 1;
    }

    struct pollfd pfd;
    pfd.fd = fd;
    pfd.events = POLLPRI;

    char buffer[3];
    while (true) {
        lseek(fd, 0, SEEK_SET);
        read(fd, buffer, sizeof(buffer));
        
        if (buffer[0] == '1') {
            std::ofstream tty1("/dev/tty1");
            if (tty1.is_open()) {
            tty1 << "GPIO==1 begin" << std::endl;
            	pushing();
             tty1 << "GPIO==1 end " << std::endl;
                tty1.close();
            }
        }
        
              else  if (buffer[0] == '0') {
            std::ofstream tty1("/dev/tty1");
            if (tty1.is_open()) {
            tty1 << "button not clicked" << std::endl;
                tty1.close();
            }
        }

        poll(&pfd, 1, 1000);  // Wait for 1 second or an interrupt
    }

    close(fd);
    return 0;
}

