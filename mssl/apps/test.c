#include <stdio.h>
#include <mssl/mssl_api.h>
#include <mssl/logs.h>

#define MAX_CORES 16
#define MSSL_CONFIG_FILE "config/mos.conf"

static int g_core_limit = 1;

static void init_monitor(mctx_t mctx)
{
  MA_LOG("Create mssl socket");
  int sock = mssl_socket(mctx, AF_INET, MOS_SOCK_SPLIT_TLS, 0);

  if (sock < 0)
    MA_LOG("Failed to create monitor raw socket!");
}

int main(int argc, char *argv[])
{
  int i;
  char *fname = MSSL_CONFIG_FILE;
  struct mssl_conf mcfg;
  mctx_t mctx_list[MAX_CORES];

  g_core_limit = get_num_cpus();

  MA_LOG1d("g_core_limit", g_core_limit);

  if (mssl_init(fname) < 0)
  {
    MA_LOG("Error in init");
    return -1;
  }

  MA_LOG("Init Success");

  for (i=0; i<g_core_limit; i++)
  {
    if ((mctx_list[i] = mssl_create_context(0)) < 0)
    {
      MA_LOG("Failed to create mssl context");
      return -1;
    }
    init_monitor(mctx_list[i]);
  }
/*
  for (i=0; i<g_core_limit; i++)
    mssl_app_join(mctx_list[i]);

  mssl_destroy();
*/
  return 0;
}
