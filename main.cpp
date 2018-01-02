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
#include <signal.h>
#include "main.h"


using namespace std;

int serial_fd;
fstream of("raw_data.csv",fstream::out);
fstream* pof = &of;
string lL;
string* lastLine = &lL;
bool lineLock = false;
bool* plL = &lineLock;

bool terminalOutput = false;
bool dataCollect = false;
bool exitRequest = false;

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
    string::size_type st = sendString.length() + 1;
    write(serial_fd,sendString.c_str(), st);
    }),
    Command("data.show", "shows data coming from the device; specifying an integer will output data only from the specified column", {}, [=](vector<string> args) {
        bool threadFlag = false;
        bool* tf = &threadFlag;	
        thread t([=](){
            string tmp = *lastLine;
            while (!*tf) {
                if (tmp != *lastLine && !*plL){
                    *plL = true;
                    cout << *lastLine << endl;
                    tmp = *lastLine;
                    *plL = false;
                }
            }
        });
        cin.get();
        threadFlag = true;
        t.join();
    }),
    Command("demo.start", "starts a demo using 1,2 or 3 columns (depending on the arguments provided)", {"columns"}, [=](vector<string> args) {
        int n = stoi(args[0]);
        cout << "Starting demo with " << n << " columns..." <<endl;
        
        
        //Data output
        bool t1Flag = false;
        bool* pt1Flag = &t1Flag;
        thread t([=](){
            string tmp = *lastLine;
            while (!*pt1Flag) {
                if (tmp != *lastLine && !*plL){
                    *plL = true;
                    //Parse string and output only Time, Oxygen percentage,
                    //Flow?
                    vector<float> inputData;
                    int pos = 0;
                    int lastpos = -1;
                    int counter = 0;
                    while((pos = lastLine->find(',',pos+1)) != string::npos){
                        string str = lastLine->substr(lastpos+1, pos);
                       
                        if ((counter - 1)%10 == ColumnValue::STATUS){
                            if (str == "ABSORB") inputData.push_back(ColumnStatus::ABSORB);
                            else if (str == "INTERME_A") inputData.push_back(ColumnStatus::INTERME_A);
                            else if (str == "INTERME_B") inputData.push_back(ColumnStatus::INTERME_B);
                            else if (str == "DESORB") inputData.push_back(ColumnStatus::DESORB);
                            else inputData.push_back(ColumnStatus::INVALID);
                        }
                        else{
                            try{
                                inputData.push_back(stof(str));
                            }catch(const invalid_argument& e){
                                inputData.push_back(0);
                            }
                        }
                        lastpos = pos;
                        counter++;                    
                    }
                    try{
                        inputData.push_back(stof(lastLine->substr(lastpos+1,lastLine->length()-1)));
                    }catch (const invalid_argument& e){
                            inputData.push_back(0);
                    }

                    if (inputData.size() > 0){
                        cout << inputData[StaticColumnValue::TIME] << ", Oxygen %: " << inputData[33] << ", T1: " << inputData[6] << ", T2: " << inputData[16] << ", T3: " << inputData[26] << endl;
                    }
                    tmp = *lastLine;
                    *plL = false;
                }
            }
        });
        
        bool t2Flag = false;
        bool* pt2Flag = &t2Flag;
        thread t2([=](){
            string sendString[3] = {"1,1,1,75,45,1.0,7,-7,60,5,255",
                                "2,1,1,75,45,1.0,5,-5,60,5,255",
                                "3,1,1,75,45,1.0,5,-5,60,5,255"};
            /*string stopString[3] = {"1,0,255",
                                "2,0,255",
                                "3,0,255"};
            */
            for (int i = 0; i < n; i++){
                cout << "SEND" << endl;
                write(serial_fd,sendString[i].c_str(),30);
                for (int j = 0; j < 1000;j++){
                    usleep(30000);
                    if (*pt2Flag){
                        //cleanup and return
                        /*for (int k = 0; k < n; k++){
                            serial.write(stopString[k].c_str(),8);
                        }*/
                        return;
                    }
                }
            }
        });
        cin.get();
        t1Flag = true;
        t2Flag = true;
        t.join();
        t2.join();
        
    }),
    Command("exit", "exits the program", {}, [=](vector<string> args) {
        exitRequest = true;        
    }),

};

int openDevice(const char* device){
     struct termios port_config; //sets up termios configuration structure for the serial port
     serial_fd = open(device, O_RDWR, O_NOCTTY);// | O_NOCTTY | O_NDELAY); //opens serial port in device slot
     if(serial_fd == -1)
     { //-1 is the error message for failed open port
           fprintf(stdout, "failed to open port\n");
     }

     if (tcgetattr(serial_fd, &port_config) < 0){
         cout << "Error getting port configuration" << endl;   
     }

     if (cfsetospeed(&port_config, B115200) < 0){
         cout << "Error setting output baud rate" << endl;
     }
     if (cfsetispeed(&port_config, B115200) < 0){
         cout << "Error setting input baud rate" << endl;
     }

     port_config.c_iflag &= ~(IXANY | IXOFF | IXON); //input flags (XON/XOFF software flow control, no NL to CR translation)
     port_config.c_oflag &= ~OPOST;
     port_config.c_lflag &= ~(ECHO | ECHOE | ICANON | ISIG); //local flags (no line processing, echo off, echo newline off, canonical mode off, extended input processing off, signal chars off)
     port_config.c_cflag &= ~(CSIZE | PARENB | CSTOPB | CRTSCTS);
     port_config.c_cflag |= (CS8 | CREAD | CLOCAL); //Control flags (local connection, enable receivingt characters, force 8 bit input)

     port_config.c_cc[VMIN]  = 1;
     port_config.c_cc[VTIME] = 0;
     tcsetattr(serial_fd, TCSANOW, &port_config); //Sets the termios struct of the file handle fd from the options defined in options. TCSAFLUSH performs the change as soon as possible.
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

void collectData(string filename){
    fstream of(filename, fstream::out);
    while(dataCollect){
        if (!*plL){
           	 
	        char c;
		int i = 0;
	        string str = "";
	        do{ 
                	if (read(serial_fd,&c,1) > 0){
	                	str += c;      	            		
			}
	        } while (c != 13 && c != 10);
	    *plL = true;
            *lastLine = string(str);	    
	    *plL = false;
            of << *lastLine << flush;
        }
	usleep(750000);
    }
}


int main(int argc, char** argv){
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
    time_t t = time(0);
    struct tm* now = localtime(&t);
    char baseFilename[17];
    sprintf(baseFilename, "%04u-%02u-%02u_%02u_%02u",(now->tm_year+1900),(now->tm_mon + 1),(now->tm_mday),(now->tm_hour),(now->tm_min));
    char filename[8+17+4]; 
    sprintf(filename,"%s-%s.csv","raw_data",baseFilename);
    cout << "Creating file: " << filename << endl;
    char buf[120];
    dataCollect = true;
    thread dataCollection(collectData,filename);
    while (!cin.eof() && !exitRequest){ 
        handle_input();
    }
    dataCollect = false;
    dataCollection.join();
    close(serial_fd);
    of.close();
}
