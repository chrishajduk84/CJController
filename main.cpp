#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <initializer_list>
#include <fstream>
#include <thread>
#include <string>
#include <string.h>
#include <sstream>
#include <sys/stat.h>
#include <dirent.h>
#include <vector>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

using namespace std;

int pid, serial_fd;
fstream of("raw_data.csv",fstream::out);
string lL;
string* lastLine = &lL;
bool lineLock = false;
bool* plL = &lineLock;

bool terminalOutput = false;
bool dataCollect = false;


/**
 * @class Command
 * @brief Describes a CLI Command that can be run in the program's repl
 */

class Command {
public:
    /**
     * @brief Constructor for a CLI command
     * @arg name Command name
     * @arg desc Command Description
     * @arg args List of argument names
     * @arg execute Function to execute when command is run, The vector of strings contains the arguments passed to the command at runtime
     */
    Command(string name, string desc, initializer_list<string> args, std::function<void(vector<string>)> execute): name(name), desc(desc), args(args), execute(execute) { }
    string name, desc;
    vector<string> args;
    std::function<void(vector<string>)> execute;
};

string questionValue(string question){
  string tmp = "";
  cout << question;
  cin >> tmp;
  return tmp;
}

vector<Command> commands = {
    Command("help", "display this help message", {}, [=](vector<string> args) {
        cout << "Commands:" << endl << endl;
        string space = string("");
        for (Command cmd : commands) {
	    int argSize = 0;
	    for (string i:cmd.args){
	         argSize += i.length();
	    }	
            space.resize(20 - cmd.name.length() + 1 + argSize, ' ');
            cout << cmd.name << " " << argSize << space << " - " << cmd.desc << endl;
        }
    }),
    Command("cartridge.add", "adds a test to the cycling jig", {}, [=](vector<string> args) {
	vector<string> data;
	    if (args.size() == 0) {
            //Ask questions:
	      	    
            data.push_back(questionValue("What cartridge (#)?"));
     	    data.push_back(questionValue("How many tests?"));
            int tests = stoi(data.at(1));
  	        for (int i = 0; i < tests; i++){
                data.push_back(questionValue("How many cycles? (#)"));
  	            data.push_back(questionValue("What is the desorption temperature? (°C)"));
  	            data.push_back(questionValue("What is the absorption temperature? (°C)"));
  	            data.push_back(questionValue("What is the heating power? (0 - 1.0)"));
  	            data.push_back(questionValue("What is the inlet pressure? (relative psi)"));
	            data.push_back(questionValue("What is the outlet pressure? (relative psi)"));
	            data.push_back(questionValue("What is the minimum heating time? (seconds)"));
	            data.push_back(questionValue("What is the minimum cooling time? (seconds)"));
            }
        } else {
           //TODO: Parse arguments
        }

	//Compute checksum and send
	unsigned char checksum = 0;
	string sendString;
	for (string i:data){
	     sendString += i + ',';
	     checksum ^= (int)stof(i) & 0xFF;
	}

    //HACKED FOR NOW
    checksum = 255;

    string buf;
	sendString += to_string(checksum);
    cout << sendString;
    string::size_type st = sendString.length()*8;
	write(serial_fd,sendString.c_str(), st);
    }),
    Command("data.show", "shows data coming from the device; specifying an integer will output data only from the specified column", {}, [=](vector<string> args) {
        bool threadFlag = false;
        bool* tf = &threadFlag;
        thread t([=](){
            string tmp = *lastLine;
            while (!*tf) {
                if (tmp != *lastLine && !*plL){
                    cout << *lastLine << endl;
                    tmp = *lastLine;
                }
            }
        });
        cin.get();
        threadFlag = true;
        t.join();
    }),
};

int openDevice(const char* device){
     struct termios port_config; //sets up termios configuration structure for the serial port
     char input[10];

     serial_fd = open(device, O_RDWR | O_NOCTTY | O_NDELAY); //opens serial port in device slot
     if(serial_fd == -1)
     { //-1 is the error message for failed open port
           fprintf(stdout, "failed to open port\n");
     }

     tcgetattr(serial_fd, &port_config);

     cfsetispeed(&port_config, B115200); //set baud input to 9600 (might be wrong?)
     cfsetospeed(&port_config, B115200); //set baud output to 9600 (might be wrong?)


     port_config.c_iflag = 0; //input flags
     port_config.c_iflag &= (IXON | IXOFF |INLCR); //input flags (XON/XOFF software flow control, no NL to CR translation)
     port_config.c_oflag = 0; //output flags
     port_config.c_lflag &= ~(ECHO | ECHONL | ICANON | IEXTEN | ISIG); //local flags (no line processing, echo off, echo newline off, canonical mode off, extended input processing off, signal chars off)
     port_config.c_cflag = 0; //control flags
     port_config.c_cflag &= (CLOCAL | CREAD | CS8); //control flags (local connection, enable receivingt characters, force 8 bit input)


     port_config.c_cc[VMIN]  = 1;
     port_config.c_cc[VTIME] = 0;

     tcsetattr(serial_fd, TCSAFLUSH, &port_config); //Sets the termios struct of the file handle fd from the options defined in options. TCSAFLUSH performs the change as soon as possible.

     pid = fork(); //splits process for recieving and sending
     cout << "Serial port opened on: " << device << endl;
}

void handle_input() {
    cout << "vivaspire$ ";
    string input;
    getline(cin, input);
    vector<string> args;
    int pos = 0;
    int lastpos = -1;
    while((pos = input.find(' ', pos+1)) != -1){
      args.push_back(input.substr(lastpos+1,pos));
      lastpos = pos;
    }
    args.push_back(input.substr(lastpos+1,pos));

    bool valid = false;
    for (Command cmd : commands) {
        if (args.size() > 0 && args[0].compare(cmd.name) == 0) {
            if (args.size() - 1 == cmd.args.size()) {
                //cout << "Executing command: " << cmd.name;
                cmd.execute(vector<string>(args.begin() + 1, args.end()));
                valid = true;
                break;
            } else {
                cout << "Usage: " << endl;
                cout << cmd.name << " ";for (string i:cmd.args) cout << i; cout << " - " << cmd.desc << endl;
            }
        }
    }
    if (!valid && !cin.eof()) {
        cout << "Executing command: " << commands[0].name;
        commands[0].execute(vector<string>());
    }
}

void collectData(){
    while(dataCollect){
        *plL = true;
	char c;
	*lastLine = "";
	while (c != '\0'){ 
             if (read(serial_fd,&c,1) > 0){
	          *lastLine += c;  
	     }
	}
        *plL = false;
        of << *lastLine; 
    }
}


int main(int argc, char** argv){
     //TODO: Find the USB device corresponding to the cycling jig, usually /dev/ttyACM0, but not guarenteed.
    int n;
    struct dirent **namelist;
    const char* sysdir = "/dev/";
    string portName;
    // Scan through /sys/class/tty - it contains all tty-devices in the system
    n = scandir(sysdir, &namelist, NULL, NULL);
    if (n < 0)
        perror("scandir");
    else {
        while (n--) {
            if (strcmp(namelist[n]->d_name,"..") && strcmp(namelist[n]->d_name,".")) {

                // Construct full absolute file path
                string devicedir = sysdir;
                devicedir += namelist[n]->d_name;
                if (devicedir.find("ACM") != string::npos){
                    portName = devicedir;
                }
            }
            free(namelist[n]);
        }
        free(namelist);
    }

     openDevice(portName.c_str());
          
     //Handle user input, handle cycling jig output
     char buf[120];
     dataCollect = true;
     thread dataCollection(collectData);
     while (!cin.eof()){ 
         handle_input();
     }
     dataCollect = false;
     dataCollection.join();
     close(serial_fd);
     of.close();
}
