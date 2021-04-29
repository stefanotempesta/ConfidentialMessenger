using Microsoft.AspNetCore.Http;
using Microsoft.AspNetCore.Mvc;
using System.Collections.Generic;
using System.Linq;
using WebChat.Data;

namespace WebChat.Controllers
{
    public class ConversationController : Controller
    {
        private readonly WebChatContext _context;

        public ConversationController(WebChatContext context)
        {
            _context = context;
        }
        public JsonResult WithContact(int contact)
        {

            if (HttpContext.Session.GetInt32("user") == null)
            {
                return Json(new { status = "error", message = "User is not logged in" });
            }

            var currentUser = _context.User.FirstOrDefault(m => m.id == HttpContext.Session.GetInt32("user"));

            var conversations = new List<Models.Conversation>();

            conversations = _context.Conversations.
                                  Where(c => (c.receiver_id == currentUser.id && c.sender_id == contact) || (c.receiver_id == contact && c.sender_id == currentUser.id))
                                  .OrderBy(c => c.created_at)
                                  .ToList();


            return Json(new { status = "success", data = conversations });
        }
    }
}
