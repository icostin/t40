#include <c41.h>
#include <hbs1.h>
#include <t40.h>

#define CMD_HELP 0
#define CMD_TEST 1

uint_t test (c41_io_t * io_p, c41_ma_t * ma_p);

/* hmain ********************************************************************/
uint8_t C41_CALL hmain (c41_cli_t * cli_p)
{
  uint_t rc, cmd;

  cmd = CMD_HELP;
  c41_io_fmt(cli_p->stdout_p, "[topor] - using engine: $s\n", t40_lib_name());

  rc = 0;

  if (cli_p->arg_n && !C41_MEM_CMP_LIT(cli_p->arg_a[0], "-t")) cmd = CMD_TEST;

  switch (cmd)
  {
  case CMD_TEST:
    rc = test(cli_p->stdout_p, cli_p->ma_p);
    break;
  default:
    c41_io_fmt(cli_p->stdout_p, "Usage: t40 [OPTIONS] script.t40\n");
    rc = 0;
    break;
  }

  return rc;
}

