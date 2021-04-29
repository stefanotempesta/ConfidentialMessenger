using Microsoft.AspNetCore.Http;
using Microsoft.AspNetCore.Mvc;
using Microsoft.EntityFrameworkCore;
using System.Linq;
using System.Threading.Tasks;
using WebChat.Data;
using WebChat.Models;
using System.Net.Http;
using Newtonsoft.Json;

using WebChat.Utils;


using System;
using System.Collections.Generic;
using System.Web;
using PusherServer;
using System.Text;

namespace WebChat.Controllers
{
    public class AuthController : Controller
    {
        private Pusher pusher;
        private User user;

        
        public IActionResult Index()
        {
            return View();
        }

        [HttpPost]
        [Route("auth/login")]
        public async Task<IActionResult> Login()
        {
            string user_name = Request.Form["username"];
            user_name = user_name.ToLower().Trim();
            if (user_name == "")
            {
                return Redirect("/");
            }

            user = new User();
            var aes = new AesEncryption();
            var encryptedUserName = aes.Encrypt(Encoding.UTF8.GetBytes(user_name));
            
            using (var httpClient = new HttpClient())
            {
                using (var response = await httpClient.GetAsync("http://localhost:1984/user/" + user_name))
                {
                     string apiResponse = await response.Content.ReadAsStringAsync();                    
                    user = JsonConvert.DeserializeObject<User>(apiResponse);
                }
            }
            /*
            using (var client = new HttpClient())
            {
                var content = new FormUrlEncodedContent(new[]
                {
                new KeyValuePair<string, string>("", "encryptedUserName")
            });
                var result = await client.PostAsync("http://localhost:1984/login", content);
                string resultContent = await result.Content.ReadAsStringAsync();
                user = JsonConvert.DeserializeObject<User>(resultContent);
                Console.WriteLine(resultContent);
            }
            */
            HttpContext.Session.SetInt32("user", user.id);

            TempData["userid"] = user.id;
            TempData["username"] = user_name;

            return Redirect("/chat");
        }

        private readonly WebChatContext _context;

        
        public AuthController(WebChatContext context)
        {
            _context = context;
            var options = new PusherOptions();
            options.Cluster = "ap1";

            pusher = new Pusher(
               "1186554",
               "224a25b62e672be2a092",
               "23ab34ee56aceeb0efc2",
               options
           );
        }

        //class constructor


        [Route("pusher/auth")]
        public JsonResult AuthForChannel(string channel_name, string socket_id)
        {
            if (HttpContext.Session.GetInt32("user") == null)
            {
                return Json(new { status = "error", message = "User is not logged in" });
            }
            var id = HttpContext.Session.GetInt32("user");
            //var currentUser = _context.User.FirstOrDefault(m => m.id == id);

            if (channel_name.IndexOf("presence") >= 0)
            {

                var channelData = new PresenceChannelData()
                {
                    user_id = HttpContext.Session.GetInt32("user").ToString(),
                    user_info = new
                    {
                        id = HttpContext.Session.GetInt32("user"),
                        name = TempData["username"]
                    },
                };

                var presenceAuth = pusher.Authenticate(channel_name, socket_id, channelData);

                return Json(presenceAuth);

            }

            if (channel_name.IndexOf(HttpContext.Session.GetInt32("user").ToString()) == -1)
            {
                return Json(new { status = "error", message = "User cannot join channel" });
            }

            var auth = pusher.Authenticate(channel_name, socket_id);

            return Json(auth);


        }
    }
}
