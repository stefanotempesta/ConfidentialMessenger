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


using namespace std;
using namespace restbed;

#define CIPHER_BLOCK_SIZE 16
#define DATA_BLOCK_SIZE 256
#define ENCRYPT_OPERATION true
#define DECRYPT_OPERATION false


oe_enclave_t* enclave = NULL;

struct user {
     int id;
     string name;
};

user* users = NULL;
int countUsers = 0;






void list_get_method_handler(const shared_ptr< Session > session)
{

    const auto& request = session->get_request();

    string data = "[";
    for (int i = 0; i < countUsers; i++)
    {

        data = data + "{\"id\":" + to_string(users[i].id) + ",\"name\":\"" +  users[i].name + "\"}";
        if (i < countUsers - 1) {
            data = data + ",";
        }
    }
    data = data + "]";
    cout << data << endl;
    session->close(OK, data, { { "Content-Length", ::to_string(data.size()) } });
}





// get the file size
int get_file_size(FILE* file, size_t* _file_size)
{
    int ret = 0;
    long int oldpos = 0;

    oldpos = ftell(file);
    ret = fseek(file, 0L, SEEK_END);
    if (ret != 0)
        goto exit;

    *_file_size = (size_t)ftell(file);
    fseek(file, oldpos, SEEK_SET);

exit:
    return ret;
}



int encrypt_file(
    bool encrypt,
    const char* password,
    const char* input_file,
    const char* output_file)
{
    oe_result_t result;
    int ret = 0;
    FILE* src_file = NULL;
    FILE* dest_file = NULL;
    unsigned char* r_buffer = NULL;
    unsigned char* w_buffer = NULL;
    size_t bytes_read;
    size_t bytes_to_write;
    size_t bytes_written;
    size_t src_file_size = 0;
    size_t src_data_size = 0;
    size_t leftover_bytes = 0;
    size_t bytes_left = 0;
    size_t requested_read_size = 0;
    encryption_header_t header;

    // allocate read/write buffers
    r_buffer = new unsigned char[DATA_BLOCK_SIZE];
    if (r_buffer == NULL)
    {
        ret = 1;
        goto exit;
    }

    w_buffer = new unsigned char[DATA_BLOCK_SIZE];
    if (w_buffer == NULL)
    {
        cerr << "Host: w_buffer allocation error" << endl;
        ret = 1;
        goto exit;
    }

    // open source and dest files
    src_file = fopen(input_file, "rb");
    if (!src_file)
    {
        cout << "Host: fopen " << input_file << " failed." << endl;
        ret = 1;
        goto exit;
    }

    ret = get_file_size(src_file, &src_file_size);
    if (ret != 0)
    {
        ret = 1;
        goto exit;
    }
    src_data_size = src_file_size;
    dest_file = fopen(output_file, "wb");
    if (!dest_file)
    {
        cerr << "Host: fopen " << output_file << " failed." << endl;
        ret = 1;
        goto exit;
    }

    // For decryption, we want to read encryption header data into the header
    // structure before calling initialize_encryptor
    if (!encrypt)
    {
        bytes_read = fread(&header, 1, sizeof(header), src_file);
        if (bytes_read != sizeof(header))
        {
            cerr << "Host: read header failed." << endl;
            ret = 1;
            goto exit;
        }
        src_data_size = src_file_size - sizeof(header);
    }

    // Initialize the encryptor inside the enclave
    // Parameters: encrypt: a bool value to set the encryptor mode, true for
    // encryption and false for decryption
    // password is provided for encryption key used inside the encryptor. Upon
    // return, _header will be filled with encryption key information for
    // encryption operation. In the case of decryption, the caller provides
    // header information from a previously encrypted file
    result = initialize_encryptor(
        enclave, &ret, encrypt, password, strlen(password), &header);
    if (result != OE_OK)
    {
        ret = 1;
        goto exit;
    }
    if (ret != 0)
    {
        goto exit;
    }

    // For encryption, on return from initialize_encryptor call, the header will
    // have encryption information. Write this header to the output file.
    if (encrypt)
    {
        header.file_data_size = src_file_size;
        bytes_written = fwrite(&header, 1, sizeof(header), dest_file);
        if (bytes_written != sizeof(header))
        {
            cerr << "Host: writting header failed. bytes_written = "
                << bytes_written << " sizeof(header)=" << sizeof(header)
                << endl;
            ret = 1;
            goto exit;
        }
    }

    leftover_bytes = src_data_size % CIPHER_BLOCK_SIZE;

    cout << "Host: leftover_bytes " << leftover_bytes << endl;

    // Encrypt each block in the source file and write to the dest_file. Process
    // all the blocks except the last one if its size is not a multiple of
    // CIPHER_BLOCK_SIZE when padding is needed
    bytes_left = src_data_size;

    if (leftover_bytes)
    {
        bytes_left = src_data_size - leftover_bytes;
    }
    requested_read_size =
        bytes_left > DATA_BLOCK_SIZE ? DATA_BLOCK_SIZE : bytes_left;
    cout << "Host: start " << (encrypt ? "encrypting" : "decrypting") << endl;

    // It loops through DATA_BLOCK_SIZE blocks one at a time then followed by
    // processing the last remaining multiple of CIPHER_BLOCK_SIZE blocks. This
    // loop makes sure all the data is processed except leftover_bytes bytes in
    // the end.
    while (
        (bytes_read = fread(
            r_buffer, sizeof(unsigned char), requested_read_size, src_file)) &&
        bytes_read > 0)
    {
        // Request for the enclave to encrypt or decrypt _input_buffer. The
        // block size (bytes_read), needs to be a multiple of CIPHER_BLOCK_SIZE.
        // In this sample, DATA_BLOCK_SIZE is used except the last block, which
        // will have to pad it to be a multiple of CIPHER_BLOCK_SIZE.
        // Eg. If testfile data size is 260 bytes, then the last block will be
        // 4 data + 12 padding bytes = 16 bytes (CIPHER_BLOCK_SIZE).
        result = encrypt_block(
            enclave, &ret, encrypt, r_buffer, w_buffer, bytes_read);
        if (result != OE_OK)
        {
            cerr << "encrypt_block error 1" << endl;
            ret = 1;
            goto exit;
        }
        if (ret != 0)
        {
            cerr << "encrypt_block error 1" << endl;
            goto exit;
        }

        bytes_to_write = bytes_read;
        // The data size is always padded to align with CIPHER_BLOCK_SIZE
        // during encryption. Therefore, remove the padding (if any) from the
        // last block during decryption.
        if (!encrypt && bytes_left <= DATA_BLOCK_SIZE)
        {
            bytes_to_write = header.file_data_size % DATA_BLOCK_SIZE;
            bytes_to_write = bytes_to_write > 0 ? bytes_to_write : bytes_read;
        }

        if ((bytes_written = fwrite(
            w_buffer, sizeof(unsigned char), bytes_to_write, dest_file)) !=
            bytes_to_write)
        {
            cerr << "Host: fwrite error  " << output_file << endl;
            ret = 1;
            goto exit;
        }

        bytes_left -= requested_read_size;
        if (bytes_left == 0)
            break;
        if (bytes_left < DATA_BLOCK_SIZE)
            requested_read_size = bytes_left;
    }

    if (encrypt)
    {
        // The CBC mode for AES assumes that we provide data in blocks of
        // CIPHER_BLOCK_SIZE bytes. This sample uses PKCS#5 padding. Pad the
        // whole CIPHER_BLOCK_SIZE block if leftover_bytes is zero. Pad the
        // (CIPHER_BLOCK_SIZE - leftover_bytes) bytes if leftover_bytes is
        // non-zero.
        size_t padded_byte_count = 0;
        unsigned char plaintext_padding_buf[CIPHER_BLOCK_SIZE];
        unsigned char ciphertext_padding_buf[CIPHER_BLOCK_SIZE];

        memset(ciphertext_padding_buf, 0, CIPHER_BLOCK_SIZE);
        memset(plaintext_padding_buf, 0, CIPHER_BLOCK_SIZE);

        if (leftover_bytes == 0)
            padded_byte_count = CIPHER_BLOCK_SIZE;
        else
            padded_byte_count = CIPHER_BLOCK_SIZE - leftover_bytes;

        cout << "Host: Working the last block" << endl;
        cout << "Host: padded_byte_count " << padded_byte_count << endl;
        cout << "Host: leftover_bytes " << leftover_bytes << endl;

        bytes_read = fread(
            plaintext_padding_buf,
            sizeof(unsigned char),
            leftover_bytes,
            src_file);
        if (bytes_read != leftover_bytes)
            goto exit;

        // PKCS5 Padding
        memset(
            (void*)(plaintext_padding_buf + leftover_bytes),
            padded_byte_count,
            padded_byte_count);

        result = encrypt_block(
            enclave,
            &ret,
            encrypt,
            plaintext_padding_buf,
            ciphertext_padding_buf,
            CIPHER_BLOCK_SIZE);
        if (result != OE_OK)
        {
            ret = 1;
            goto exit;
        }
        if (ret != 0)
        {
            goto exit;
        }

        bytes_written = fwrite(
            ciphertext_padding_buf,
            sizeof(unsigned char),
            CIPHER_BLOCK_SIZE,
            dest_file);
        if (bytes_written != CIPHER_BLOCK_SIZE)
            goto exit;
    }

    cout << "Host: done  " << (encrypt ? "encrypting" : "decrypting") << endl;

    // close files
    fclose(src_file);
    fclose(dest_file);

exit:
    delete[] r_buffer;
    delete[] w_buffer;
    cout << "Host: called close_encryptor" << endl;

    result = close_encryptor(enclave);
    if (result != OE_OK)
    {
        ret = 1;
    }
    return ret;
}

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
        cout << "in loop" + to_string(i) << endl;
        if (users[i].name.compare(namestr) == 0) {
            cout << "found match" << endl;
            matchindex = i;
        }
    }

    if (matchindex >= 0) {
        data = data + "{\"id\":" + to_string(users[matchindex].id) + ",\"name\":\"" + users[matchindex].name + "\"}";
    }
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
          //  return 1;
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
           // return 1;
        }
        remove(decrypted_file);
    }

    cout << data << endl;
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
            cout << "Unusual username:" + users[i].name << endl;
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

    // encrypt a file
 /*   cout << "Host: encrypting file:" << input_file
        << " -> file:" << encrypted_file << endl;
    ret = encrypt_file(
        ENCRYPT_OPERATION, "anyPasswordYouLike", input_file, encrypted_file);
    if (ret != 0)
    {
        cerr << "Host: processFile(ENCRYPT_OPERATION) failed with " << ret
            << endl;
        return 1;
    }
*/
    // Decrypt a file
 /*   cout << "Host: decrypting file:" << encrypted_file
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
*/
    ret = read_in_host_file();
    //read_file(enclave);
    if (ret != 0)
    {
        cerr << "Host: read_in_host_file failed with " << ret
            << endl;
        return 1;
    }

    auto resource = make_shared< Resource >();
    resource->set_path("/user/{name: .*}");
    resource->set_method_handler("GET", get_method_handler);

 /*   auto resource = make_shared< Resource >();
    resource->set_path("/login");
    resource->set_method_handler("POST", post_method_handler);
*/
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