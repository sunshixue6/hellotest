#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <fnmatch.h>

typedef struct {
    int sec_of_day;         // 用于排序：HH*3600+MM*60+SS
    unsigned long seq;      // 同一秒内保持原始读取顺序（稳定排序效果）
    char time_str[9];       // "HH:MM:SS"
    double soc;
} Row;

static int extract_time_hms(const char *line, char out[9], int *sec_of_day) {
    // 在整行中找第一个符合 HH:MM:SS 的位置
    const size_t n = strlen(line);
    for (size_t i = 0; i + 7 < n; i++) {
        if (isdigit((unsigned char)line[i]) &&
            isdigit((unsigned char)line[i+1]) &&
            line[i+2] == ':' &&
            isdigit((unsigned char)line[i+3]) &&
            isdigit((unsigned char)line[i+4]) &&
            line[i+5] == ':' &&
            isdigit((unsigned char)line[i+6]) &&
            isdigit((unsigned char)line[i+7])) {

            int hh = (line[i]-'0')*10 + (line[i+1]-'0');
            int mm = (line[i+3]-'0')*10 + (line[i+4]-'0');
            int ss = (line[i+6]-'0')*10 + (line[i+7]-'0');

            if (hh < 0 || hh > 23 || mm < 0 || mm > 59 || ss < 0 || ss > 59)
                continue;

            memcpy(out, &line[i], 8);
            out[8] = '\0';
            if (sec_of_day) *sec_of_day = hh * 3600 + mm * 60 + ss;
            return 1;
        }
    }
    return 0;
}

static int extract_soc_pcb_value(const char *line, double *val) {
    const char *p = strstr(line, "SOC_Core[");
    if (!p) return 0;
    p += strlen("SOC_Core[");
    char *endp = NULL;
    double v = strtod(p, &endp);
    if (endp == p) return 0;
    if (*endp != ']') return 0;
    *val = v;
    return 1;
}

static void chomp(char *s) {
    size_t n = strlen(s);
    while (n && (s[n-1] == '\n' || s[n-1] == '\r')) s[--n] = '\0';
}

static int cmp_row(const void *a, const void *b) {
    const Row *ra = (const Row*)a;
    const Row *rb = (const Row*)b;
    if (ra->sec_of_day != rb->sec_of_day)
        return (ra->sec_of_day < rb->sec_of_day) ? -1 : 1;
    // 同一秒按读取顺序输出，避免“同秒乱序”
    if (ra->seq != rb->seq)
        return (ra->seq < rb->seq) ? -1 : 1;
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s out.csv\n", argv[0]);
        return 1;
    }

    // 先收集所有记录，再排序输出
    Row *rows = NULL;
    size_t len = 0, cap = 0;
    unsigned long seq = 0;

    DIR *dir;
    struct dirent *entry;
    char line[8192];

    //dir = opendir("loginput");
    dir = opendir("logno");
    if (dir == NULL) {
        perror("opendir loginput");
        return 1;
    }

    const char *log_pattern = "XVM_MCU-*";

    while ((entry = readdir(dir)) != NULL) {
        if (fnmatch(log_pattern, entry->d_name, 0) == 0) {
            char filepath[8192];
            snprintf(filepath, sizeof(filepath), "logno/%s", entry->d_name);

            FILE *fp = fopen(filepath, "r");
            if (!fp) {
                perror(filepath);
                continue;
            }

            while (fgets(line, sizeof(line), fp)) {
                chomp(line);

                char t[9];
                int sod = 0;
                double soc = 0.0;

                if (extract_time_hms(line, t, &sod) && extract_soc_pcb_value(line, &soc)) {
                    if (len == cap) {
                        size_t newcap = cap ? cap * 2 : 4096;
                        Row *tmp = (Row*)realloc(rows, newcap * sizeof(Row));
                        if (!tmp) {
                            fprintf(stderr, "Out of memory\n");
                            free(rows);
                            fclose(fp);
                            closedir(dir);
                            return 1;
                        }
                        rows = tmp;
                        cap = newcap;
                    }

                    rows[len].sec_of_day = sod;
                    rows[len].seq = seq++;
                    memcpy(rows[len].time_str, t, 9);
                    rows[len].soc = soc;
                    len++;
                }
            }

            fclose(fp);
        }
    }
    closedir(dir);

    // 排序
    qsort(rows, len, sizeof(Row), cmp_row);

    // 输出 CSV
    const char *out_csv = argv[1];
    FILE *out = fopen(out_csv, "w");
    if (!out) {
        perror("fopen out.csv");
        free(rows);
        return 1;
    }

    fprintf(out, "time,soc_pcb\n");
    for (size_t i = 0; i < len; i++) {
        fprintf(out, "%s,%g\n", rows[i].time_str, rows[i].soc);
    }

    fclose(out);
    free(rows);
    return 0;
}