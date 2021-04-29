using Microsoft.AspNetCore.Http;
using Microsoft.AspNetCore.Mvc;
using Microsoft.EntityFrameworkCore;
using System.Linq;
using System.Threading.Tasks;
using WebChat.Data;
using WebChat.Models;


using System;
using System.Collections.Generic;
using System.Web;
using PusherServer;


namespace WebChat.Controllers
{
    public class AuthController : Controller
    {
        private Pusher pusher;

        
        public IActionResult Index()
        {
            return View();
        }

        [HttpPost]
        public async Task<IActionResult> Login()
        {

            string user_name = Request.Form["username"];

            if (user_name.Trim() == "")
            {
                return Redirect("/");
            }

            var user = await _context.User.FirstOrDefaultAsync(m => m.name == user_name);


            if (user == null)
            {
                user = new User { name = user_name };
                _context.Add(user);
                await _context.SaveChangesAsync();


            }

            HttpContext.Session.SetInt32("user", user.id);


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

        private bool UserExists(int id)
        {
            return _context.User.Any(e => e.id == id);
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
            var currentUser = _context.User.FirstOrDefault(m => m.id == id);

            if (channel_name.IndexOf("presence") >= 0)
            {

                var channelData = new PresenceChannelData()
                {
                    user_id = currentUser.id.ToString(),
                    user_info = new
                    {
                        id = currentUser.id,
                        name = currentUser.name
                    },
                };

                var presenceAuth = pusher.Authenticate(channel_name, socket_id, channelData);

                return Json(presenceAuth);

            }

            if (channel_name.IndexOf(currentUser.id.ToString()) == -1)
            {
                return Json(new { status = "error", message = "User cannot join channel" });
            }

            var auth = pusher.Authenticate(channel_name, socket_id);

            return Json(auth);


        }
    }
}
