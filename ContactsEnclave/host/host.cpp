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

#include "contactsenclave_u.h"

#include <string>
#include <memory>
#include <cstdlib>
#include <restbed>
#include "contactsserver.h"


using namespace std;
using namespace restbed;

struct user {
     int id;
     string name;
};

user* users = NULL;
int countUsers = 0;

//Method handling the request for all the contacts in the chat application, returning a JSON object containing the ID and username for each user
void list_get_method_handler(const shared_ptr< Session > session)
{

    const auto& request = session->get_request();

    string data = "[";
    for (int i = 0; i < countUsers; i++)
    {
        if (users[i].name == "") {
            continue;
        }
        data = data + "{\"id\":" + to_string(users[i].id) + ",\"name\":\"" +  users[i].name + "\"}";
        if (i < countUsers - 1) {
            data = data + ",";
        }
    }
    data = data + "]";
    session->close(OK, data, { { "Content-Length", ::to_string(data.size()) } });
}

//Method handling the GET request for a particular username returning a JSON with the id and username
void get_method_handler(const shared_ptr< restbed::Session > session)
{
    int ret = 0;
    const char* encrypted_file = "./out.encrypted";
    const char* decrypted_file = "./out.decrypted";
    const auto& request = session->get_request();
    int matchindex = -1;

    string namestr = request->get_path_parameter("name");
    string data = "";
    for (int i = 0; i < countUsers; i++)
    {
        if (users[i].name.compare(namestr) == 0) {
            matchindex = i;
        }
    }
    //If username in request matches an existing username, the response will contain the matched data
    if (matchindex >= 0) {
        data = data + "{\"id\":" + to_string(users[matchindex].id) + ",\"name\":\"" + users[matchindex].name + "\"}";
    }
	//If username in request does not match an existing username, a new user will be created first and the data added 
	//to the encrypted contact list file
    else {
        countUsers++;
        data = data + "{\"id\":" + to_string(countUsers) + ",\"name\":\"" + namestr + "\"}";
        users[countUsers - 1].id = countUsers;
        users[countUsers - 1].name = namestr;

        cout << "Host: decrypting file:" << encrypted_file
            << " to file:" << decrypted_file << endl;

        ret = encrypt_file(
            DECRYPT_OPERATION,
            "anyPasswordYouLike",
            encrypted_file,
            decrypted_file);
        if (ret != 0)
        {
            cerr << "Host: processFile(DECRYPT_OPERATION) failed with " << ret
                << endl;
        }

        string filename = decrypted_file;
        ofstream file_out;
        file_out.open(filename, ios_base::app);
        file_out << to_string(countUsers) + "," + namestr << endl;
        file_out.close();

        cout << "Host: encrypting file:" << decrypted_file
            << " -> file:" << encrypted_file << endl;
        ret = encrypt_file(
            ENCRYPT_OPERATION, "anyPasswordYouLike", decrypted_file, encrypted_file);
        if (ret != 0)
        {
            cerr << "Host: processFile(ENCRYPT_OPERATION) failed with " << ret
                << endl;
        }
       remove(decrypted_file);
    }

    session->close(OK, data, { { "Content-Length", ::to_string(data.size()) } });
}

int read_in_host_file() {
    // Decrypt a file
    const char* encrypted_file = "./out.encrypted";
    const char* decrypted_file = "./out.decrypted";
    int ret = 0;
    cout << "Host: decrypting file:" << encrypted_file
        << " to file:" << decrypted_file << endl;

    ret = encrypt_file(
        DECRYPT_OPERATION,
        "anyPasswordYouLike",
        encrypted_file,
        decrypted_file);
    if (ret != 0)
    {
        cerr << "Host: processFile(DECRYPT_OPERATION) failed with " << ret
            << endl;
        return 1;
    }


    string line;
    ifstream myfile("./out.decrypted");
    if (myfile.is_open())
    {
        while (getline(myfile, line)) {
            countUsers++;
        }
    }
    else cout << "Unable to open file" << endl;
    users = new user[100];
    myfile.clear();
    myfile.seekg(0);
    for (int i = 0; i < countUsers; i++)
    {
        myfile >> users[i].id >> users[i].name;
        try {
            users[i].name = users[i].name.substr(1, users[i].name.length() - 1);
        }
        catch (std::out_of_range& exception) {
            users[i].name = "";
        }

    }
    myfile.close();
   remove(decrypted_file);
    return 0;
}

int main(int argc, const char* argv[])
{
    oe_result_t result;
    int ret = 0;
    const char* input_file = "./testfile";
    const char* encrypted_file = "./out.encrypted";
    const char* decrypted_file = "./out.decrypted";
    uint32_t flags = OE_ENCLAVE_FLAG_DEBUG;

    cout << "Host: enter main" << endl;
    cout << "Host: create enclave for image:" << argv[2] << endl;
    result = oe_create_contactsenclave_enclave(
        argv[2], OE_ENCLAVE_TYPE_SGX, flags, NULL, 0, &enclave);
    if (result != OE_OK)
    {
        cerr << "oe_create_fileencryptor_enclave() failed with " << argv[0]
            << " " << result << endl;
        return 1;
    }


    ret = read_in_host_file();
    if (ret != 0)
    {
        cerr << "Host: read_in_host_file failed with " << ret
            << endl;
        return 1;
    }

    auto resource = make_shared< Resource >();
    resource->set_path("/user/{name: .*}");
    resource->set_method_handler("GET", get_method_handler);

    auto resource2 = make_shared< Resource >();
    resource2->set_path("/users");
    resource2->set_method_handler("GET", list_get_method_handler);
	
	auto ssl_settings = make_shared< SSLSettings >( );
    ssl_settings->set_http_disabled( true );
    ssl_settings->set_private_key( Uri( "file:///tmp/server.key" ) );
    ssl_settings->set_certificate( Uri( "file:///tmp/server.crt" ) );
    ssl_settings->set_temporary_diffie_hellman( Uri( "file:///tmp/dh768.pem" ) );

    auto settings = make_shared< Settings >();
    settings->set_port(1984);
    settings->set_default_header("Connection", "close");
	//settings->set_ssl_settings( ssl_settings );

    Service service;
    service.publish(resource);
    service.publish(resource2);
    service.start(settings);

    return EXIT_SUCCESS;

 

}
