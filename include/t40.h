#ifndef _T40_H_
#define _T40_H_

#include <c41.h>

#if T40_STATIC
# define T40_API
#elif T40_DL_BUILD
# define T40_API C41_DL_EXPORT
#else
# define T40_API C41_DL_IMPORT
#endif

#define T40_OK                                  0x00
#define T40_FAILED                              0x01 // generic fail code
#define T40_NO_CODE                             0x02 // not implemented
#define T40_MEM_ERROR                           0x03 // check world_p->ma_rc for ma error code
#define T40_NO_FIELDS                           0x04 // get/set field on object that cannot have fields
#define T40_FIELD_LIM                           0x05 // reached limit of fields in object

#define T40_BUG_REF                             0xFE // ref-overflow
#define T40_BUG                                 0xFF

#define T40_LOG_NONE                            0x00
#define T40_LOG_ERROR                           0x01
#define T40_LOG_WARNING                         0x02
#define T40_LOG_INFO                            0x03

/* built-in object indexes */
enum t40_ox_enum
{
  T40_OX_ERROR = 0, // reserved index to signify error occured
  T40_OX_NULL,
  T40_OX_FALSE,
  T40_OX_TRUE,
  T40_OX_OBJECT_CLASS,
  T40_OX_CLASS_CLASS,
  T40_OX_MODULE_CLASS,
  T40_OX_FUNC_CLASS,
  T40_OX_NULL_CLASS,
  T40_OX_CBA_CLASS,
  T40_OX_BA_CLASS,
  T40_OX_ID_CLASS,
  T40_OX_BOOL_CLASS,
  T40_OX_REF_INT_CLASS,
  T40_OX_CORE,

  T40_OX__COUNT
};

#define T40_IS_IREF(_r) ((_r) & 1)
#define T40_IS_NREF(_r) (((_r) & 3) == 0)

#define T40_NREF_INDEX(_r) ((uint_t) (_r) >> 2)
#define T40_IREF_VALUE(_r) ((int32_t) (_r) >> 1)

#define T40_IREF_MAKE(_int31) (((int32_t) (_int31) << 1) | 1)

typedef struct t40_ba_s                         t40_ba_t;
typedef struct t40_class_s                      t40_class_t;
typedef struct t40_flow_s                       t40_flow_t;
typedef struct t40_id_s                         t40_id_t;
typedef struct t40_init_s                       t40_init_t;
typedef struct t40_kvp_s                        t40_kvp_t;
typedef struct t40_module_s                     t40_module_t;
typedef struct t40_obj_s                        t40_obj_t;
typedef uint32_t                                t40_ox_t; // object index
typedef int32_t                                 t40_ref_t;
typedef struct t40_world_s                      t40_world_t;

typedef uint_t (* C41_CALL t40_action_f) (t40_world_t * world_p, 
                                          t40_obj_t * obj_p);

struct t40_kvp_s
{
  t40_ref_t     key;
  t40_ref_t     value;
};

struct t40_obj_s // a hash where keys are identifiers
{
  t40_class_t * class_p;
  t40_kvp_t * field_a;
  uint32_t field_n; // up to 1G fields
  uint32_t rc;
};

struct t40_module_s
{
  t40_obj_t     obj;
  t40_ox_t      id_ox;
};

struct t40_class_s
{
  t40_obj_t     obj;
  t40_kvp_t *   method_a;
  uint_t        method_n;
  t40_ox_t      self_ox;
  t40_ox_t      super_ox;
  t40_ox_t      id_ox;
  size_t        instance_size;
  t40_action_f  finish;
    /*< finish functions must take into account the fact that some references
     *  from the instance might be invalid at the time of calling if the
     *  instance is garbage-collected (or the world is destroyed) 
     */
  t40_action_f  mark_finish_deps; 
    /*< this function is called during garbage collection or during 
     * world-destroy to flag the other objects that are used during the 
     * finish of an instance of this class */
};

struct t40_ba_s
{
  t40_obj_t     obj;
  uint8_t *     a;
  size_t        n, m;
  uint8_t       allocated;
  uint8_t       writable;
  uint8_t       resizeable;
};


struct t40_id_s
{
  t40_obj_t     obj;
  c41_rbtree_node_t  rbn;
  t40_ox_t      cba_ox;
  t40_ox_t      self_ox;
};

struct t40_world_s
{
  union
  {
    void * *      vpa;
    t40_obj_t * * pa;
    uintptr_t *   nf_oxa; // next free - object index array
  }             obj;

  t40_flow_t *  flow_p; // active flow
  c41_np_t      flow_list;
  c41_rbtree_t  id_cba_tree;

  t40_ox_t      lim_ox; // capacity of obj tables
  t40_ox_t      uninit_ox; // lowest uninited obj index
  t40_ox_t      ff_ox; // obj index of first free slot

  c41_ma_t      ma;

  uint_t        lrc; // last return code
  uint_t        ma_rc;
  uint_t        finish_rc;

  t40_class_t   object_class;
  t40_class_t   class_class;
  t40_class_t   module_class;
  t40_class_t   func_class;
  t40_class_t   null_class;
  t40_class_t   bool_class;
  t40_class_t   cba_class;
  t40_class_t   ba_class;
  t40_class_t   id_class;
  t40_class_t   ref_int_class;

  t40_obj_t     null_obj;
  t40_obj_t     false_obj;
  t40_obj_t     true_obj;
  t40_module_t  core_module;

  c41_io_t *    log_io_p;
  uint8_t       log_level;
  uint8_t       is_ending;
};

struct t40_flow_s
{
  t40_world_t * world_p;
  c41_np_t      links;
};

struct t40_init_s
{
  c41_ma_t * ma_p;
  c41_io_t * log_p;
  uint8_t log_level;
};

T40_API char const * C41_CALL t40_lib_name ();
T40_API char const * C41_CALL t40_status_name (uint_t sc);

T40_API uint_t C41_CALL t40_world_create (t40_world_t * world_p, t40_init_t * init_p);
T40_API uint_t C41_CALL t40_world_finish (t40_world_t * world_p);

/* t40_cba_static ***********************************************************/
T40_API t40_ref_t C41_CALL t40_cba_static
(
  t40_world_t * world_p,
  uint8_t const * data_a,
  size_t data_n
);
/*^ on success it returns the allocated object index
 *  on error returns 0 and world_p->lrc has the reason
 */

/* t40_cba_static_cstr ******************************************************/
T40_API t40_ref_t C41_CALL t40_cba_static_cstr
(
  t40_world_t * world_p,
  char const * data_azt // NUL-ended string
);

#define T40_CBA_STATIC_STR_LIT(_world_p, _buf) \
  (t40_cba_static((_world_p), (uint8_t const *) (_buf), sizeof(_buf) - 1))

/* t40_id_fsd ***************************************************************
 * id from static data
 * returns a ref to an indentifier matching given data; if there was no
 * identifier matching, one is created (including its cba);
 * if the cba is created, its fields will point to data buffer
 */
T40_API t40_ref_t C41_CALL t40_id_fsd
(
  t40_world_t * world_p,
  uint8_t const * data_a,
  size_t data_n
);
#define T40_ID_FSD(_w, _sl) (t40_id_fsd((_w), (uint8_t const *) (_sl), sizeof(_sl) - 1))

/* t40_ref ******************************************************************/
T40_API uint_t C41_CALL t40_ref
(
  t40_world_t * world_p,
  t40_ref_t obj_r
);

/* t40_deref ****************************************************************/
T40_API uint_t C41_CALL t40_deref
(
  t40_world_t * world_p,
  t40_ref_t obj_r
);

/* t40_set_field ************************************************************/
T40_API uint_t C41_CALL t40_set_field (t40_world_t * world_p, t40_ref_t obj_r, 
                                       t40_ref_t field_id_r, t40_ref_t value_r);


#endif /* _T40_H_ */

