// Copyright (c) Open Enclave SDK contributors.
// Licensed under the MIT License.

#include <assert.h>
#include <limits.h>
#include <openenclave/host.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>
#include "../shared.h"

#include "fileencryptor_u.h"

#include <string>
#include <memory>
#include <cstdlib>
#include <restbed>


using namespace std;
using namespace restbed;

oe_enclave_t* enclave = NULL;

struct user {
     int id;
     char name[256];
};

user* users = NULL;
int countUsers = 0;


void read_in_host_file() {
    string line;
    ifstream myfile("./contacts.txt");
    if (myfile.is_open())
    {
        while (getline(myfile, line)) {
            countUsers++;
        }
    }
    else cout << "Unable to open file";
    users = new user[countUsers];
    myfile.clear();
    myfile.seekg(0);
    for(int i = 0; i < countUsers; i++)
        {
        myfile >> users[i].id >> users[i].name;
        }
    myfile.close();
}

void get_method_handler(const shared_ptr< restbed::Session > session)
{
    /* sample format: TODO: info will be coming from enclave and the name value  is encrypted
    "user": {
         "id": 1,
         "name": ""
    }
    */

    const auto& request = session->get_request();

    //const string body = "single method";
    const string body = "\"user\": {\"id\": 1, \"name\": " + request->get_path_parameter("name") + "\"";
    session->close(OK, body, { { "Content-Length", ::to_string(body.size()) } });
}

void list_get_method_handler(const shared_ptr< Session > session)
{
    /* sample format: TODO: info will be coming from enclave and the name value  is encrypted
   "users": [{
        "id": 1,
        "name": ""
   }, {
        "id": 2,
        "name": ""
   }
   ]
   */
    const auto& request = session->get_request();
    //const string body = "users method";
    const string body = "\"users\": [{\"id\": 1, \"name\" : \"Ryan\"}, {\"id\": 2, \"name\" : \"Rey\"}]";
    string data = "";
    for (int i = 0; i < countUsers; i++)
    {
        data = data + to_string(users[i].id) + "," +  users[i].name + "\n";
    }

    session->close(OK, data, { { "Content-Length", ::to_string(body.size()) } });
}


int main(int argc, const char* argv[])
{
    oe_result_t result;
    int ret = 0;

    uint32_t flags = OE_ENCLAVE_FLAG_DEBUG;

    cout << "Host: enter main" << endl;

    cout << "Host: create enclave for image:" << argv[2] << endl;
    result = oe_create_fileencryptor_enclave(
        argv[2], OE_ENCLAVE_TYPE_SGX, flags, NULL, 0, &enclave);
    if (result != OE_OK)
    {
        cerr << "oe_create_fileencryptor_enclave() failed with " << argv[0]
             << " " << result << endl;
        ret = 1;
        cout << "Host: error creating enclave" << endl;
    }

    read_in_host_file();
    //read_file(enclave);


    auto resource = make_shared< Resource >();
    resource->set_path("/user/{name: .*}");
    resource->set_method_handler("GET", get_method_handler);

    auto resource2 = make_shared< Resource >();
    resource2->set_path("/users");
    resource2->set_method_handler("GET", list_get_method_handler);


    auto settings = make_shared< Settings >();
    settings->set_port(1984);
    settings->set_default_header("Connection", "close");

    Service service;
    service.publish(resource);
    service.publish(resource2);
    service.start(settings);

    return EXIT_SUCCESS;





}
