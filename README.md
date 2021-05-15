# ConfidentialMessenger
ConfidentialMessenger is a web chat application that utilizes the confidential computing capabilities of Azure to protect data (in this case the contacts list) while in use. It modifies and builds on the chat application described in https://pusher.com/tutorials/chat-aspnet/  The ConfidentialMessenger application can be configred to use either a confidential database in Azure or a confidential enclave in an Azure virtual machine to store the chat application's registered usernames.
The ConfidentialMessenger is written on the ASP.Net Core platform using C# as programming language. 

# A. Using Confidential SQL Database in Azure

One way to incorporate confidential computing in your system is to use an SQL database in Azure with confidential computing support. To learn more about confidential SQL databases, refer to the following document.
https://docs.microsoft.com/en-us/sql/relational-databases/security/encryption/always-encrypted-enclaves?view=sql-server-ver15

1. The first step in replicating this system is to provision a confidential SQL database in Azure. Steps 1 and 2 of the following tutorial will show you how to setup an Azure SQL database with an Always Encrypted enclave.
https://docs.microsoft.com/en-us/azure/azure-sql/database/always-encrypted-enclaves-getting-started

2. Clone/download the code in this repository and open the project/solution for ConfidentialMessenger in your IDE.

3. In the appsettings.json file, plugin the values for the confidential SQL database you created in step 1 to the ConnectionString/WebChatContext property. You can find the actual values for the connection string in the database's overview section in the Azure portal.
Example:
   Server=<server name and port; check for the value in the Azure portal>;Initial Catalog=<database name>;Persist Security Info=False;User ID=<admin username>;Password=<password>;MultipleActiveResultSets=False;Encrypt=True;TrustServerCertificate=False;Connection Timeout=30;Column Encryption Setting = Enabled;Attestation Protocol = AAS; Enclave Attestation Url = <attestation URL you created in Step 1>;
Also, set the value of the ContactListSource property to "DB".
  
4. Execute a Powershell command to create the database schema for ConfidentialMessenger in the database created. In your Visual Studio IDE, go to Tools -> NuGet Package Manager -> Package Manager Console. Create the application tables in your database by running the following command:
   Update-Database
   This command will create the Users and Conversations table. The Users table is the contact list and would be used to store an encrypted user name. The Conversations table will store the chat messages.

5. Provision enclave-enabled keys for your database and then set the name column in the User Table to be Always Encrypted. Use Steps 4 and 5 in the following tutorial as guide on how to create enclave-enabled keys and set a specified column to be encrypted using those keys.
https://docs.microsoft.com/en-us/azure/azure-sql/database/always-encrypted-enclaves-getting-started

6. The setup of the system is complete and the application can be run and tested locally. Use the Start Debugging/Start without Debugging command on Visual Studio which should open a page in your browser showing you the login page for the ConfidentialMessenger.  

# B. Using Confidential Enclave in Azure VM
You can also use a confidential enclave in a special Azure VM to store sensitive data. In this application, the contacts list data is stored in a VM enclave. The ContactsEnclave C++ application uses the openenclave architecture to store and process the contacts list and acts as a REST server to client applications requesting such data. 

# I. ContactsEnclave
First, we will setup the ContactsEnclave REST server, the C++ application deployed in the Azure VM which leverages confidential computing support and used to manage and store the contacts list, considered to be sensitive data. 
1. The first step is to provision an Azure virtual machine that supports confidential computing. Follow the instructions in this tutorial (https://docs.microsoft.com/en-us/azure/confidential-computing/quick-create-portal) up to the installation of the Open Enclave SDK. For the purpose of the ConfidentialMessenger system, the VM size selected was Standard DC1s_v2 (1 vcpus, 4 GiB memory) and the VM operating system used was Linux (ubuntu 18.04), though other options are possible depending on need and preference.

2. Connect to the server using Putty or other SSH terminal, clone the ConfidentialMessenger repository in your home directory.
3. Navigate to the ContactsEnclave -> host folder, and then clone and build the Restbed repository.
    https://github.com/Corvusoft/restbed#build
4. Navigate to the ContactsEnclave directory and run the following commands:
    . /opt/openenclave/share/openenclave/openenclaverc
    make build
    export LD_LIBRARY_PATH="./ContactsEnclave/host/restbed/distribution/library"
    nohup make run
5. The ContactsEnclave server will now be running. Verify by opening another terminal and running the following curl command
    curl -w'\n' -v -XGET 'http://localhost:1984/users'
    
# II. ConfidentialMessenger
The ConfidentialMessenger is the .NET Core/C# web chat application. It is designedto be deployed alongside the ContactsEnclave application.
1. In Visual Studio, open the project/solution inside the ConfidentialMessenger folder.
2. In the appsettings.json file, set the value of the ContactListSource property to "VM". The application needs an SQL database to store the Conversations data, so a ConnectionString to a SQL Server database needs to be provided.
  Ex.  "WebChatContext": "Server=tcp:confmsgrdbserver.database.windows.net,1433;Initial Catalog=confmsgrdb;Persist Security Info=False;User ID={username};Password={password};MultipleActiveResultSets=False;Encrypt=True;TrustServerCertificate=False;Connection Timeout=30;"
3. Execute a Powershell command to create the database schema for ConfidentialMessenger in the database specified. In your Visual Studio IDE, go to Tools -> NuGet Package Manager -> Package Manager Console. Create the tables in your database by running the following command:
   Update-Database
   This command will create the Users and Conversations table. The Users table is not going to be used as users data is going to be stored in the enclave. The Conversations table will store the chat messages.
4. Publish the application to a local folder.
5. Using an FTP client, copy the publish folder and contents to your home directory in the VM
6. Create an NGINX service to host the web application. Refer to the following resource:
    https://hbhhathorn.medium.com/install-an-asp-net-core-web-api-on-linux-ubuntu-18-04-and-host-with-nginx-and-ssl-2ed9df7371fb
5. Open a browser and navigate to the web application.

