#include <c41.h>
#include <t40.h>

/* mkid *********************************************************************/
static t40_ref_t mkid (c41_io_t * err_io_p, t40_world_t * world_p, 
                       char const * s)
{
  t40_ref_t r;
  r = t40_id_fsd(world_p, (uint8_t const *) s, C41_STR_LEN(s));
  if (!r)
  {
    c41_io_fmt(err_io_p, "Error: failed to create identifier '$s' ($s = $i)\n",
               s, t40_status_name(world_p->lrc), world_p->lrc);
  }
  else
  {
    c41_io_fmt(err_io_p, "mkid('$s'): ref=$Xd ptr=$Xp\n", s, r, 
               world_p->obj.vpa[T40_NREF_INDEX(r)]);
  }

  return r;
}

/* print_id_tree ************************************************************/
static void print_id_tree (c41_io_t * err_io_p, t40_world_t * world_p, 
                           t40_id_t * id_p)
{
  t40_ba_t * cba_p;
  //c41_io_fmt(err_io_p, "print_id_tree($p)\n", id_p);
  if (id_p->rbn.left)
    print_id_tree(err_io_p, world_p, 
                  C41_FIELD_TO_OBJECT(t40_id_t, rbn, id_p->rbn.left));
  cba_p = world_p->obj.vpa[id_p->cba_ox];
  // c41_io_fmt(err_io_p, "printing (id:$p) cba_ox=$Xd cba_p=$XG4p, a=$XG4p, n=$Xz\n", 
  //            id_p, id_p->cba_ox, cba_p, cba_p->a, cba_p->n);
  c41_io_fmt(err_io_p, "$.*s [$Dd]\n", cba_p->n, cba_p->a, cba_p->obj.rc);
  if (id_p->rbn.right)
    print_id_tree(err_io_p, world_p, 
                  C41_FIELD_TO_OBJECT(t40_id_t, rbn, id_p->rbn.right));
}

/* print_ids ****************************************************************/
static void print_ids (c41_io_t * err_io_p, t40_world_t * world_p)
{
  print_id_tree(err_io_p, world_p, 
                C41_FIELD_TO_OBJECT(t40_id_t, rbn, world_p->id_cba_tree.root));
}

/* test *********************************************************************/
uint_t test (c41_io_t * io_p, c41_ma_t * ma_p)
{
  t40_world_t world;
  t40_init_t init;
  c41_ma_counter_t mac;
  size_t mem_limit = 0x10000;
  uint_t c;
  t40_ref_t r;

  c41_io_fmt(io_p, "testing...\n");
  c41_ma_counter_init(&mac, ma_p, mem_limit, mem_limit, C41_SSIZE_MAX);

  C41_VAR_ZERO(init);
  init.ma_p = &mac.ma;
  init.log_level = T40_LOG_INFO;
  init.log_p = io_p;

  c = t40_world_create(&world, &init);
  if (c)
  {
    c41_io_fmt(io_p, "Error: world_create failed ($s, code $Ui)\n",
               t40_status_name(c), c);
    return 1;
  }

  r = T40_CBA_STATIC_STR_LIT(&world, "object");
  if (!r)
  {
    c41_io_fmt(io_p, "Error: failed to get cba for 'object' ($s, code $Ui)\n",
               t40_status_name(world.lrc), world.lrc);
    return 1;
  }

  r = T40_ID_FSD(&world, "asdfasdf");
  if (!r)
  {
    c41_io_fmt(io_p, "Error: failed to get id for 'asdfasdf' ($s, code $Ui)\n",
               t40_status_name(world.lrc), world.lrc);
    return 1;
  }
  c41_io_fmt(io_p, "asdfasdf ref=$Xd, ox=$Hd\n", r, T40_NREF_INDEX(r));

  r = mkid(io_p, &world, "abc");
  r = mkid(io_p, &world, "def");
  r = mkid(io_p, &world, "ghi");
  r = mkid(io_p, &world, "jkl");
  r = mkid(io_p, &world, "mno");
  r = mkid(io_p, &world, "pq");
  r = mkid(io_p, &world, "rstu");
  r = mkid(io_p, &world, "vw");
  r = mkid(io_p, &world, "xyz");

  c41_io_fmt(io_p, "de-ref=$Xd, ox=$Hd\n", r, T40_NREF_INDEX(r));

  t40_deref(&world, r);

  print_ids(io_p, &world);

  c = t40_world_finish(&world);
  if (c)
  {
    c41_io_fmt(io_p, "Error: world_finish failed ($s, code $Ui)\n",
               t40_status_name(c), c);
    return 1;
  }

  if (mac.count || mac.total_size)
  {
    c41_io_fmt(io_p, "Error: MEMORY LEAK => "
               "block count: $Xz, total size: $Xz\n", 
               mac.count, mac.total_size);
    return 1;
  }

  return 0;
}
