using Microsoft.EntityFrameworkCore;
using WebChat.Models;

namespace WebChat.Data
{
    public class WebChatContext : DbContext
    {
        public WebChatContext(DbContextOptions<WebChatContext> options)
            : base(options)
        {
        }

        public WebChatContext()
            
        {
        }

        public DbSet<User> User { get; set; }
        public DbSet<Conversation> Conversations { get; set; }
    }
}
