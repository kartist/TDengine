#include "taos.h"
#include "taoserror.h"
#include "os.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

int numSuperTables = 8;
int numChildTables = 4;
int numRowsPerChildTable = 2048;

void shuffle(char**lines, size_t n)
{
  if (n > 1)
  {
    size_t i;
    for (i = 0; i < n - 1; i++)
    {
      size_t j = i + rand() / (RAND_MAX / (n - i) + 1);
      char* t = lines[j];
      lines[j] = lines[i];
      lines[i] = t;
    }
  }
}

static int64_t getTimeInUs() {
  struct timeval systemTime;
  gettimeofday(&systemTime, NULL);
  return (int64_t)systemTime.tv_sec * 1000000L + (int64_t)systemTime.tv_usec;
}

int main(int argc, char* argv[]) {
  TAOS_RES *result;
  const char* host = "127.0.0.1";
  const char* user = "root";
  const char* passwd = "taosdata";

  taos_options(TSDB_OPTION_TIMEZONE, "GMT-8");
  TAOS* taos = taos_connect(host, user, passwd, "", 0);
  if (taos == NULL) {
    printf("\033[31mfailed to connect to db, reason:%s\033[0m\n", taos_errstr(taos));
    exit(1);
  }

  char* info = taos_get_server_info(taos);
  printf("server info: %s\n", info);
  info = taos_get_client_info(taos);
  printf("client info: %s\n", info);
  result = taos_query(taos, "drop database if exists db;");
  taos_free_result(result);
  usleep(100000);
  result = taos_query(taos, "create database db precision 'ms';");
  taos_free_result(result);
  usleep(100000);

  (void)taos_select_db(taos, "db");

  time_t ct = time(0);
  int64_t ts = ct * 1000;
  char* lineFormat = "sta%d,t0=true,t1=127i8,t2=32767i16,t3=%di32,t4=9223372036854775807i64,t9=11.12345f32,t10=22.123456789f64,t11=\"binaryTagValue\",t12=L\"ncharTagValue\" c0=true,c1=127i8,c2=32767i16,c3=2147483647i32,c4=9223372036854775807i64,c5=254u8,c6=32770u16,c7=2147483699u32,c8=9223372036854775899u64,c9=11.12345f32,c10=22.123456789f64,c11=\"binaryValue\",c12=L\"ncharValue\" %lldms";

  char** lines = calloc(numSuperTables * numChildTables * numRowsPerChildTable, sizeof(char*));
  int l = 0;
  for (int i = 0; i < numSuperTables; ++i) {
    for (int j = 0; j < numChildTables; ++j) {
      for (int k = 0; k < numRowsPerChildTable; ++k) {
        char* line = calloc(512, 1);
        snprintf(line, 512, lineFormat, i, j, ts + 10 * l);
        lines[l] = line;
        ++l;
      }
    }
  }
  //shuffle(lines, numSuperTables * numChildTables * numRowsPerChildTable);

  printf("%s\n", "begin taos_insert_lines");
  int64_t  begin = getTimeInUs();
  int32_t code = taos_insert_lines(taos, lines, numSuperTables * numChildTables * numRowsPerChildTable);
  int64_t end = getTimeInUs();
  printf("code: %d, %s. time used: %"PRId64"\n", code, tstrerror(code), end-begin);

  return 0;
}
