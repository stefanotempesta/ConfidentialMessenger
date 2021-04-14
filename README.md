# ConfidentialMessenger

ConfidentialMessenger is a simple web chat application written in C#. Its main feature is that the list of contacts is stored in a confidential SQL database in Azure. To learn more about confidential SQL databases, refer to the following document.
https://docs.microsoft.com/en-us/sql/relational-databases/security/encryption/always-encrypted-enclaves?view=sql-server-ver15

1. The first step in replicating this system is to provision a confidential SQL database using Steps 1 and 2 of the following tutorial.
https://docs.microsoft.com/en-us/azure/azure-sql/database/always-encrypted-enclaves-getting-started

2. Clone/download the code in this repository and open the project in your IDE.

3. In the appsettings.json file, plugin the values for the confidential SQL database you created in step 1 to the ConnectionString/WebChatContext property. 
Example:
   Server=<server name and port; check for the value in the Azure portal>;Initial Catalog=<database name>;Persist Security Info=False;User ID=<admin username>;Password=<password>;MultipleActiveResultSets=False;Encrypt=True;TrustServerCertificate=False;Connection Timeout=300;Column Encryption Setting = Enabled;Attestation Protocol = AAS; Enclave Attestation Url = <attestation URL you created in Step 1>;
  
4. In your Visual Studio IDE, go to Tools -> NuGet Package Manager -> Package Manager Console. Create the application tables in your database by running the following command:
   Update-Database

5. Using Sql Server Management Studio, provision enclave-enabled keys for your database and then encrypt the name column in the User Table. Use Steps 4 and 5 in the tutorial as guide. 
https://docs.microsoft.com/en-us/azure/azure-sql/database/always-encrypted-enclaves-getting-started

6. The setup of the system is complete and the application can now be run locally.
