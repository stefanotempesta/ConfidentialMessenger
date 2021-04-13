﻿using Microsoft.AspNetCore.Http;
using Microsoft.AspNetCore.Mvc;
using Microsoft.EntityFrameworkCore;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using WebChat.Data;
using WebChat.Models;
using PusherServer;

namespace WebChat.Controllers
{
    public class ChatController : Controller
    {
        private readonly WebChatContext _context;

        public ChatController(WebChatContext context)
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
        
        public async Task<IActionResult> Index()
        {

            if (HttpContext.Session.GetInt32("user") == null)
            {
                return Redirect("/");
            }
            var id = HttpContext.Session.GetInt32("user");

            ViewBag.allUsers = await _context.User.Where(u => u.id != id).ToListAsync();
            ViewBag.currentUser = await _context.User.FirstOrDefaultAsync(m => m.id == id);

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

            var currentUser = _context.User.FirstOrDefault(m => m.id == HttpContext.Session.GetInt32("user"));
            var contact = Convert.ToInt32(Request.Form["contact"]);
            string socket_id = Request.Form["socket_id"];
            Conversation convo = new Conversation
            {
                sender_id = currentUser.id,
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
