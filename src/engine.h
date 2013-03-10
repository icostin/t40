#ifndef _T40_ENGINE_H_
#define _T40_ENGINE_H_

#include <t40.h>

#define INITIAL_OBJECT_COUNT 0x100

#if _DEBUG
#define LL(_level, ...) \
  do { \
    if (world_p->log_level < (_level)) break; \
    c41_io_fmt(world_p->log_io_p, "[t40] $s ($s:$Ui:$s): ", \
               (_level) == T40_LOG_ERROR ? "Error" : \
               (_level == T40_LOG_WARNING ? "Warning" : "Info"), \
               __FILE__, __LINE__, __FUNCTION__); \
    c41_io_fmt(world_p->log_io_p, __VA_ARGS__); \
    c41_io_fmt(world_p->log_io_p, "\n"); \
  } while (0)
#define A(_cond) \
  do { \
    if ((_cond)) break; \
    LE("ASSERTION FAILED ($s)", #_cond); \
    goto l_bug; \
  } while (0)
#else
#define LL(_level, ...)
#define A(_cond)
#endif

#define LE(...) LL(T40_LOG_ERROR, __VA_ARGS__)
#define LW(...) LL(T40_LOG_WARNING, __VA_ARGS__)
#define LI(...) LL(T40_LOG_INFO, __VA_ARGS__)

#define H LI("");

typedef struct t40_kvn_s                        t40_kvn_t;
struct t40_kvn_s
{
  c41_rbtree_node_t rb;
  t40_ref_t key;
  t40_ref_t value;
};

#define T40_FLOW_FROM_LINKS(_l) \
  ((t40_flow_t *) (C41_PTR_OFS(_l, -(C41_FIELD_OFS(t40_flow_t, links)))))

#define OX_TO_REF(_ox) (((int32_t) (_ox)) << 2)

/* ox_alloc *****************************************************************/
C41_LOCAL t40_ox_t C41_CALL ox_alloc (t40_world_t * world_p);
/* returns the allocated index into world_p->obj_pa
 * on error returns 0 and world_p->lrc has the error code
 */

/* ox_free ******************************************************************/
C41_LOCAL void C41_CALL ox_free (t40_world_t * world_p, t40_ox_t ox);

/* ox_ref *******************************************************************/
C41_INLINE uint_t C41_CALL ox_ref (t40_world_t * world_p, t40_ox_t ox)
{
  if (!++world_p->obj.pa[ox]->rc)
  {
    LE("ref-overflow on object $Xd", ox);
    return T40_BUG_REF;
  }
  return 0;
}

/* ox_deref *****************************************************************/
C41_LOCAL uint_t C41_CALL ox_deref (t40_world_t * world_p, t40_ox_t ox);

/* obj_alloc ****************************************************************/
C41_LOCAL void * C41_CALL obj_alloc
(
  t40_world_t * world_p,
  t40_class_t * class_p
);

/* obj_create ***************************************************************
 * returns 0 when the object could not be created (world.lrc has the error)
 */
C41_LOCAL t40_ox_t C41_CALL obj_create
(
  t40_world_t * world_p,
  t40_class_t * class_p
);

/* class_init ***************************************************************
 * initialises a regular class (whose type is class_class)
 */
C41_LOCAL uint_t C41_CALL class_init
(
  t40_world_t * world_p,
  t40_ox_t self_ox,
  t40_ox_t super_ox,
  size_t instance_size,
  t40_action_f finish,
  t40_action_f mark_finish_deps
);

#endif /* _T40_ENGINE_H_ */

