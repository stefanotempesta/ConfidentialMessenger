using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;

using System.IO;
using System.Security.Cryptography;
using System.Text;

namespace WebChat.Utils
{
    public class AesEncryption
    {
        private byte[] key = Encoding.UTF8.GetBytes("Tw0O5kCeBgYjaaf7ATM4gDcpDEvAPr4Qql7xTIwVMsc=");
        private byte[] iv = Encoding.UTF8.GetBytes("+RG8XccgibEqsLG5n0jqvA==");

        public byte[] GenerateRandomNumber(int length)
        {
            using (var randomNumberGenerator = new RNGCryptoServiceProvider())
            {
                var randomNumber = new byte[length];
                randomNumberGenerator.GetBytes(randomNumber);
                return randomNumber;
            }
        }

        public byte[] Decrypt(byte[] dataToDecrypt)
        {
            using (var aes = new AesCryptoServiceProvider())
            {
                aes.Mode = CipherMode.CBC;
                aes.Padding = PaddingMode.PKCS7;

                using (var memoryStream = new MemoryStream())
                {
                    var cryptoStream =
                    new CryptoStream(memoryStream,
                    aes.CreateDecryptor(),
                    CryptoStreamMode.Write);

                    cryptoStream.Write(dataToDecrypt, 0,
dataToDecrypt.Length);
                    cryptoStream.FlushFinalBlock();
                    return memoryStream.ToArray();
                }
            }
        }


        public byte[] Encrypt(byte[] dataToEncrypt)
        {
            using (var aes = new AesCryptoServiceProvider())
            {
                aes.Mode = CipherMode.CBC;
                aes.Padding = PaddingMode.PKCS7;
                using (var memoryStream = new MemoryStream())
                {
                    var cryptoStream = new CryptoStream(
                    memoryStream,
                    aes.CreateEncryptor(),
                    CryptoStreamMode.Write);
                    cryptoStream.Write(dataToEncrypt, 0,
                    dataToEncrypt.Length);
                    cryptoStream.FlushFinalBlock();
                    return memoryStream.ToArray();
                }
            }
        }
    }
}
