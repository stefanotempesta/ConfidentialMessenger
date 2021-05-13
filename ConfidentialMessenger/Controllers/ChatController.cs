using Microsoft.AspNetCore.Http;
using Microsoft.AspNetCore.Mvc;
using Microsoft.EntityFrameworkCore;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using WebChat.Data;
using WebChat.Models;
using PusherServer;
using System.Net.Http;
using Newtonsoft.Json;
using WebChat.Utils;
using System.Text;
using Microsoft.Extensions.Configuration;


namespace WebChat.Controllers
{
    public class ChatController : Controller
    {
        private readonly WebChatContext _context;
        private readonly IConfiguration _config;
        public ChatController(WebChatContext context, IConfiguration iConfig)
        {
            _context = context;
            _config = iConfig;
            var options = new PusherOptions();
            options.Cluster = "ap1";
            pusher = new Pusher(
               "1186554",
               "224a25b62e672be2a092",
               "23ab34ee56aceeb0efc2",
               options
           );
        }
        
        public async Task<IActionResult> Index()
        {

            if (HttpContext.Session.GetInt32("user") == null)
            {
                return Redirect("/");
            }
            var id = HttpContext.Session.GetInt32("user");

            List<User> userList = new List<User>();
            User currentUser = null;

            string contactsSource = _config.GetValue<string>("ContactListSource");
            if (contactsSource == "VM")
            {
                using (var httpClient = new HttpClient())
                {
                    using (var response = await httpClient.GetAsync("http://localhost:1984/users"))
                    {
                        string apiResponse = await response.Content.ReadAsStringAsync();

                        userList = JsonConvert.DeserializeObject<List<User>>(apiResponse);

                    }
                }

                currentUser = userList.SingleOrDefault(r => r.id == id);
                if (currentUser != null)
                {
                    userList.Remove(currentUser);
                }
            } else
            {
                userList = await _context.User.Where(u => u.id != id).ToListAsync();
                currentUser = await _context.User.FirstOrDefaultAsync(m => m.id == id);
            }
                
            ViewBag.allUsers = userList;
            ViewBag.currentid = currentUser.id;
            ViewBag.currentname = currentUser.name;

            return View();
        }



        [Route("contact/conversations/{contact}")]
        public JsonResult ConversationWithContact(int contact)
        {
            if (HttpContext.Session.GetInt32("user") == null)
            {
                return Json(new { status = "error", message = "User is not logged in" });
            }
            var id = HttpContext.Session.GetInt32("user");
            var conversations = new List<Models.Conversation>();

            conversations = _context.Conversations.
                                  Where(c => (c.receiver_id == id && c.sender_id == contact) || (c.receiver_id == contact && c.sender_id == id))
                                  .OrderBy(c => c.created_at)
                                  .ToList();

            return Json(new { status = "success", data = conversations });
        }

        [HttpPost]
        [Route("send_message")]
        public JsonResult SendMessage()
        {
            if (HttpContext.Session.GetInt32("user") == null)
            {
                return Json(new { status = "error", message = "User is not logged in" });
            }

            var id = HttpContext.Session.GetInt32("user");

            // var currentUser = _context.User.FirstOrDefault(m => m.id == id);
           /// var currentUser = TempData["userData"] as User;
            var contact = Convert.ToInt32(Request.Form["contact"]);
            string socket_id = Request.Form["socket_id"];
            Conversation convo = new Conversation
            {
                sender_id = (int)id,
                message = Request.Form["message"],
                receiver_id = contact
            };

            _context.Add(convo);
            _context.SaveChanges();

            return Json(convo);


            
        }

    [HttpPost]
    [Route("message_delivered/{message_id}")]   
    public JsonResult MessageDelivered(int message_id)
        {
            Conversation convo = null;

                convo = _context.Conversations.FirstOrDefault(c => c.id == message_id);
            if (convo != null)
                {
                    convo.status = Conversation.messageStatus.Delivered;
                _context.Entry(convo).State = EntityState.Modified;
                _context.SaveChanges();
                }


            return Json(convo);
        }

        private String getConvoChannel(int user_id, int contact_id)
        {
            if (user_id > contact_id)
            {
                return "private-chat-" + contact_id + "-" + user_id;
            }
            return "private-chat-" + user_id + "-" + contact_id;
        }


        private Pusher pusher;
        
       

       
       

    }
}
