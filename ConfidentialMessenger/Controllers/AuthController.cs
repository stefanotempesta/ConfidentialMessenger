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
using Microsoft.Extensions.Configuration;

namespace WebChat.Controllers
{
    public class AuthController : Controller
    {
        private readonly IConfiguration _config;
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
            
            string contactsSource = _config.GetValue<string>("ContactListSource");

            if (contactsSource == "VM") { 
            using (var httpClient = new HttpClient())
            {
                    // using (var response = await httpClient.GetAsync("http://localhost:1984/user/" + user_name))
                    using (var response = await httpClient.GetAsync(_config.GetValue<string>("ContactListServerHostName") + "user/" + user_name))
                    {
                     string apiResponse = await response.Content.ReadAsStringAsync();                    
                    user = JsonConvert.DeserializeObject<User>(apiResponse);
                }
            }
            } else
            {
                user = await _context.User.FirstOrDefaultAsync(m => m.name == user_name);
                if (user == null)
            {
                user = new User { name = user_name };
                _context.Add(user);
                await _context.SaveChangesAsync();

            }
            }
            HttpContext.Session.SetInt32("user", user.id);

            TempData["userid"] = user.id;
            TempData["username"] = user_name;

            return Redirect("/chat");
        }

        private readonly WebChatContext _context;
      
        public AuthController(WebChatContext context, IConfiguration iConfig )
        {
            _context = context;
            _config = iConfig;
            var options = new PusherOptions();
            //  options.Cluster = "ap1";
            options.Cluster = _config.GetValue<string>("PusherCluster");
            /*
                        pusher = new Pusher(
                           "1186554",
                           "224a25b62e672be2a092",
                           "23ab34ee56aceeb0efc2",
                           options
                       );
            */
            pusher = new Pusher(
               _config.GetValue<string>("PusherAppId"),
               _config.GetValue<string>("PusherAppKey"),
               _config.GetValue<string>("PusherAppSecret"),
               options
           );
        }

        private bool UserExists(int id)
        {
            return _context.User.Any(e => e.id == id);
        }

        [Route("pusher/auth")]
        public JsonResult AuthForChannel(string channel_name, string socket_id)
        {
            if (HttpContext.Session.GetInt32("user") == null)
            {
                return Json(new { status = "error", message = "User is not logged in" });
            }
            var id = HttpContext.Session.GetInt32("user");

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
