
#include <argp.h>

struct arguments {
   char *hostname;
   char *port;
   bool verbose;
   bool autostart;
} arguments;

// set up command line option checking using argp.h
const char *argp_program_version = "mohses_ros_bridge v0.1.0";
const char *argp_program_bug_address = "<rainer@uw.edu>";

static char doc[] = 
    "Bridge module for data exchange between MoHSES and a ROS instance.";

static char args_doc[] = "";
static struct argp_option options[] = {
    { "host",  'h', "HOST", 0, "Host IP address"},
    { "port",  'p', "PORT", 0, "Host port"},
    { "autostart",'a', 0, 0, "Autostart monitor"},
    { "verbose",  'v', 0, 0, "Print extra data"},
    { 0 }
};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
   struct arguments *arguments = (struct arguments*)(state->input);

   switch (key) {
      case 'h':
         arguments->hostname = arg;
         break;
      case 'p':
         arguments->port = arg;
         break;
      case 'a':
         arguments->autostart = true;
         break;
      case 'v':
         arguments->verbose = true;
         break;
      case ARGP_KEY_ARG: 
         argp_usage (state);
         break;
      default: 
         return ARGP_ERR_UNKNOWN;
   }
   return 0;
}

static struct argp argp = { options, parse_opt, args_doc, doc};
