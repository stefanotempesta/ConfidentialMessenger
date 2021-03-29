using Microsoft.AspNetCore.Http;
using Microsoft.AspNetCore.Mvc;
using Microsoft.EntityFrameworkCore;
using System.Linq;
using System.Threading.Tasks;
using WebChat.Data;
using WebChat.Models;


namespace WebChat.Controllers
{
    public class AuthController : Controller
    {
        public IActionResult Index()
        {
            return View();
        }

        private readonly WebChatContext _context;

        public AuthController(WebChatContext context)
        {
            _context = context;
        }

        private bool UserExists(int id)
        {
            return _context.User.Any(e => e.id == id);
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
    }
}
