# ConfidentialMessenger
ConfidentialMessenger is a simple web chat application written in C# to demonstrate the use of Azure confidential computing technologies. There are two versions of ConfidentialMessenger based on where the contacts list is stored. The first one uses a confidential database in Azure. The second version uses a confidential enclave in an Azure virtual machine.

A. Using Confidential SQL Database in Azure

In this version of ConfidentialMessenger, the list of contacts is stored in a confidential SQL database in Azure. To learn more about confidential SQL databases, refer to the following document.
https://docs.microsoft.com/en-us/sql/relational-databases/security/encryption/always-encrypted-enclaves?view=sql-server-ver15

1. The first step in replicating this system is to provision a confidential SQL database in Azure. Learn how to do so using Steps 1 and 2 of the following tutorial.
https://docs.microsoft.com/en-us/azure/azure-sql/database/always-encrypted-enclaves-getting-started

2. Clone/download the code in this repository and open the project/solution for ConfidentialMessengerDB in your IDE.

3. In the appsettings.json file, plugin the values for the confidential SQL database you created in step 1 to the ConnectionString/WebChatContext property. You can find the actual values for the connection string in the database's overview section in the Azure portal.
Example:
   Server=<server name and port; check for the value in the Azure portal>;Initial Catalog=<database name>;Persist Security Info=False;User ID=<admin username>;Password=<password>;MultipleActiveResultSets=False;Encrypt=True;TrustServerCertificate=False;Connection Timeout=30;Column Encryption Setting = Enabled;Attestation Protocol = AAS; Enclave Attestation Url = <attestation URL you created in Step 1>;
  
4. Execute a Powershell command to create the database schema for ConfidentialMessenger in the database created. In your Visual Studio IDE, go to Tools -> NuGet Package Manager -> Package Manager Console. Create the application tables in your database by running the following command:
   Update-Database
   This command will create the Users and Conversations table. The Users table is the contact list and would be used to store an encrypted user name. The Conversations table will store the chat messages.

5. Provision enclave-enabled keys for your database and then set the name column in the User Table to be Always Encrypted. Use Steps 4 and 5 in the following tutorial as guide on how to create enclave-enabled keys and set a specified column to be encrypted using those keys.
https://docs.microsoft.com/en-us/azure/azure-sql/database/always-encrypted-enclaves-getting-started

6. The setup of the system is complete and the application can be run and tested locally. Use the Start Debugging/Start without Debugging command on Visual Studio which should open a page in your browser showing you the login page for the ConfidentialMessenger.

B. Using Confidential Enclave in Azure VM
This version of ConfidentialMessenger uses a confidential enclave in a special Azure VM to store contacts data. The VM hosts an application that uses openenclave SDK architecture to store and serve the contact list data to client applications. The data server application is written in C++ and uses restbed SDK to implement REST server functionalities. The web application is written in C# and an Azure SL database is used to store the chat messages.
TODO: Instructions on setting up, building and running this version

