#include <boost/program_options.hpp>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <string>
#include <vector>
#include <unistd.h>

#include <includes/data_types.h>
#include <includes/transactions.h>

namespace po = boost::program_options;
using namespace std;
using namespace diamond;

DStringList cloudList;
DStringList edgeList;
DStringList deviceList;
DList deviceStatusList;
DCounter uniqueId; 

int main(int argc, char ** argv) {
    string configPrefix;
    string myName, key;

    // gather commandline options
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help",
         "produce help message")
        ("key",
         po::value<std::string>(&key)->required(),
         "iot user name")
        ("name",
         po::value<std::string>(&myName)->required(),
         "edge name")
        ("config",
         po::value<std::string>(&configPrefix)->required(),
         "frontend config file prefix (required)");
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    if (vm.count("help")) {
        cout << desc << endl;
        exit(1);
    }
    po::notify(vm);

    //set up diamond
    DiamondInit(configPrefix, 1, 0);
    StartTxnManager();

    // Map edge state
    DObject::Map(cloudList, key + "iot:cloudList");
    DObject::Map(edgeList, key + "iot:edgeList");
    DObject::Map(deviceList, key + "iot:deviceList");
    DObject::Map(deviceStatusList, key + "iot:deviceStatusList");
    DObject::Map(uniqueId, key + "iot:uniqueId");

    // Add Edge
    execute_txn([myName] () {
        if (edgeList.Index(myName) == -1) edgeList.Append(myName);
    },
    // txn callback: If transaction fails, exit
    [myName] (bool committed) { 
        if (!committed) exit(1);
    });
    // Set up our reactive display
    uint64_t reactive_id =
    reactive_txn([myName] () {
        // print out current device id and cloud and edge
        cout << "Unique Id: " << uniqueId.Value() << endl;
        cout << "Cloud Nodes: ";
        for (auto &p : cloudList.Members())
              cout << p << " ";
        cout << endl;
        cout << "Edge Nodes: ";
        for (auto &p : edgeList.Members())
              cout << p << " ";
        cout << endl;
        cout << "Devices: " << endl;
        for (auto &p : deviceList.Members()){
		cout << "Name:" << p << " ";
		int index = deviceList.Index(p);
		if (index != -1)
			cout << "Status:" << deviceStatusList.Value(index);
		cout << endl;
	}
        cout << endl;
        cout << "Enter device name: " << flush;
    });

    // Cycle on user input to change device status
    while (1) {
	string deviceName;
	uint64_t status;
	cin >> deviceName;
        cout << "Enter device status: " << flush;
	cin >> status;
        // Add device
        execute_txn([myName,deviceName,status] () {
	    int index = deviceList.Index(deviceName);
            if (index != -1)
		deviceStatusList.Insert(index,status);
        },
        // txn callback: If transaction fails, exit
        [] (bool committed) { 
            if (!committed) exit(1);
        });
    }
    return 0;
}
