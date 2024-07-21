#include <stdio.h>
#include <unistd.h>
#include <syslog.h>
#include <libgen.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char *argv[]) 
{
    FILE* fp = NULL;
    char* file_path = NULL;
    int rc = 0;

    openlog(NULL, 0, LOG_USER);

    if(argc != 3) {
        syslog(LOG_ERR, "incorrect arguments, app expects 2 string arguments 1->path, 2->string to write");
        rc = 1;
        goto end;
    }
    
    file_path = strdup(argv[1]); /* since dirname strips file name at end*/
    if (access(dirname(argv[1]), F_OK) == -1 && rc == 0) {
        syslog(LOG_ERR, "cannot create file, directory doesnt exist %s", dirname(argv[1]));
        rc = 1;
    }
    else {
        fp = fopen(file_path, "w");
        if(fp) {
            fwrite(argv[2], sizeof(char), strlen(argv[2]), fp);
            fflush(fp);
            fclose(fp);
            syslog(LOG_DEBUG, "Writing %s to %s", argv[2], file_path);
            rc = 0;
        }
        else {
            syslog(LOG_ERR, "cannot open file to write %s", file_path);
            rc = 1;
        }
    }
end:
    if(file_path)
        free(file_path);
    closelog();
    return rc;
}