#include <glib.h>

#define CONFIG_FILE_NAME = "phoenix.config"
#define NVRAM_SIZE = "nvramcapacity"

gboolean status = FALSE;

char *getProperty(char *key){
	status = gboolean g_key_file_load_from_file (&keyfile, CONFIG_FILE_NAME,G_KEY_FILE_NONE, NULL);
	char *value = g_key_file_get_string(&keyfile, NULL,NVRAM_SIZE,NULL);
	return value;
}



int main(int argc, char *argv[]){

char * value = getProperty("test");
printf("%s\n", value);

}
