#include "engine.h"

/* id_ref_cmp ***************************************************************/
static uint_t C41_CALL id_cba_cmp
(
  void * key,
  void * payload,
  void * context
)
{
  c41_u8an_t const * key_anp = key;
  //t40_id_t const * id_node = C41_FIELD_TO_OBJECT(t40_id_t, cba_ox, payload);
  t40_ox_t const * cba_ox = payload;
  t40_world_t * world_p = context;
  t40_ba_t * cba_p = world_p->obj.vpa[*cba_ox];
  int cmp;

  if (key_anp->n != cba_p->n)
    return (key_anp->n < cba_p->n) ? C41_RBTREE_LEFT : C41_RBTREE_RIGHT;

  cmp = C41_MEM_COMPARE(key_anp->a, cba_p->a, key_anp->n);
  return cmp ? (cmp < 0 ? C41_RBTREE_LEFT : C41_RBTREE_RIGHT)
             : C41_RBTREE_EQUAL;
}

/* nop_finish ***************************************************************/
C41_LOCAL uint_t C41_CALL nop_finish (t40_world_t * world_p, t40_obj_t * obj_p)
{
  (void) world_p;
  (void) obj_p;
  return T40_OK;
}

/* default_mark_finish_deps *************************************************/
C41_LOCAL uint_t C41_CALL default_mark_finish_deps
(
  t40_world_t * world_p,
  t40_obj_t * obj_p
)
{
  (void) (world_p);
  obj_p->class_p->obj.rc += 1;
  return 0;
}

/* id_mark_finish_deps ******************************************************/
C41_LOCAL uint_t C41_CALL id_mark_finish_deps
(
  t40_world_t * world_p,
  t40_obj_t * obj_p
)
{
  t40_id_t * id_p = (t40_id_t *) obj_p;
  obj_p->class_p->obj.rc += 1;
  ox_ref(world_p, id_p->cba_ox);
  return 0;
}

/* id_finish ****************************************************************/
C41_LOCAL uint_t C41_CALL id_finish (t40_world_t * world_p, t40_obj_t * obj_p)
{
  t40_id_t * id_p = (t40_id_t *) obj_p;
  t40_ba_t * cba_p;
  c41_u8an_t key_an;
  c41_rbtree_path_t path;
  int frc;

  cba_p = world_p->obj.vpa[id_p->cba_ox];
  key_an.a = cba_p->a;
  key_an.n = cba_p->n;
  LI("finishing id '$.*s'...", key_an.n, key_an.a);
  frc = c41_rbtree_find(&path, &world_p->id_cba_tree, &key_an);
  if (frc) return T40_BUG;
  c41_rbtree_delete(&path);
  return ox_deref(world_p, id_p->cba_ox);
}

/* t40_world_create *********************************************************/
T40_API uint_t C41_CALL t40_world_create
(
  t40_world_t * world_p,
  t40_init_t * init_p
)
{
  uint_t ma_rc, rc;
  t40_flow_t * flow_p;
  t40_ox_t ox;

  C41_VAR_ZERO(*world_p);

  if (init_p->log_level)
  {
    world_p->log_io_p = init_p->log_p;
    world_p->log_level = init_p->log_level;
    LI("inited logger");
  }
  world_p->ma = *init_p->ma_p;

  c41_rbtree_init(&world_p->id_cba_tree, id_cba_cmp, world_p);
  c41_dlist_init(&world_p->flow_list);

  ma_rc = C41_VAR_ALLOC(&world_p->ma, world_p->obj.pa, INITIAL_OBJECT_COUNT);
  if (ma_rc)
  {
    LE("cannot allocate initial table of object pointers ($s, code $Ui)",
       c41_ma_status_name(ma_rc), ma_rc);
    goto l_ma_error;
  }

  world_p->lim_ox = INITIAL_OBJECT_COUNT;
  world_p->uninit_ox = T40_OX__COUNT;
  world_p->ff_ox = 0;

  ma_rc = C41_VAR_ALLOC1Z(&world_p->ma, flow_p);
  if (ma_rc)
  {
    LE("cannot allocate initial flow ($s, code $Ui)",
       c41_ma_status_name(ma_rc), ma_rc);
    goto l_ma_error;
  }
  world_p->flow_p = flow_p;
  C41_DLIST_APPEND(world_p->flow_list, flow_p, links);

  world_p->obj.pa[T40_OX_NULL                   ] = &world_p->null_obj;
  world_p->obj.pa[T40_OX_FALSE                  ] = &world_p->false_obj;
  world_p->obj.pa[T40_OX_TRUE                   ] = &world_p->true_obj;
  world_p->obj.pa[T40_OX_OBJECT_CLASS           ] = &world_p->object_class.obj;
  world_p->obj.pa[T40_OX_CLASS_CLASS            ] = &world_p->class_class.obj;
  world_p->obj.pa[T40_OX_NULL_CLASS             ] = &world_p->null_class.obj;
  world_p->obj.pa[T40_OX_FUNC_CLASS             ] = &world_p->func_class.obj;
  world_p->obj.pa[T40_OX_CBA_CLASS              ] = &world_p->cba_class.obj;
  world_p->obj.pa[T40_OX_BA_CLASS               ] = &world_p->ba_class.obj;
  world_p->obj.pa[T40_OX_ID_CLASS               ] = &world_p->id_class.obj;
  world_p->obj.pa[T40_OX_BOOL_CLASS             ] = &world_p->bool_class.obj;
  world_p->obj.pa[T40_OX_REF_INT_CLASS          ] = &world_p->ref_int_class.obj;
  world_p->obj.pa[T40_OX_MODULE_CLASS           ] = &world_p->module_class.obj;
  world_p->obj.pa[T40_OX_CORE                   ] = &world_p->core_module.obj;

  rc = class_init(world_p, T40_OX_OBJECT_CLASS, T40_OX_NULL,
                  sizeof(t40_obj_t), nop_finish, default_mark_finish_deps);
  if (rc) goto l_error;
  rc = class_init(world_p, T40_OX_CLASS_CLASS, T40_OX_OBJECT_CLASS,
                  sizeof(t40_class_t), nop_finish, default_mark_finish_deps);
  if (rc) goto l_error;
  rc = class_init(world_p, T40_OX_MODULE_CLASS, T40_OX_OBJECT_CLASS,
                  sizeof(t40_module_t), nop_finish, default_mark_finish_deps);
  if (rc) goto l_error;
  rc = class_init(world_p, T40_OX_FUNC_CLASS, T40_OX_OBJECT_CLASS,
                  sizeof(t40_obj_t), nop_finish, default_mark_finish_deps);
  if (rc) goto l_error;
  rc = class_init(world_p, T40_OX_NULL_CLASS, T40_OX_OBJECT_CLASS,
                  sizeof(t40_obj_t), nop_finish, default_mark_finish_deps);
  if (rc) goto l_error;
  rc = class_init(world_p, T40_OX_CBA_CLASS, T40_OX_OBJECT_CLASS,
                  sizeof(t40_ba_t), nop_finish, default_mark_finish_deps);
  if (rc) goto l_error;
  rc = class_init(world_p, T40_OX_BA_CLASS, T40_OX_CBA_CLASS,
                  sizeof(t40_ba_t), nop_finish, default_mark_finish_deps);
  if (rc) goto l_error;
  rc = class_init(world_p, T40_OX_ID_CLASS, T40_OX_OBJECT_CLASS,
                  sizeof(t40_ba_t), id_finish, id_mark_finish_deps);
  if (rc) goto l_error;
  rc = class_init(world_p, T40_OX_BOOL_CLASS, T40_OX_OBJECT_CLASS,
                  sizeof(t40_obj_t), nop_finish, default_mark_finish_deps);
  if (rc) goto l_error;
  rc = class_init(world_p, T40_OX_REF_INT_CLASS, T40_OX_OBJECT_CLASS,
                  0, nop_finish, default_mark_finish_deps);
  if (rc) goto l_error;

  world_p->null_obj.class_p = &world_p->null_class;
  world_p->null_obj.rc = (1 << 31);

  world_p->false_obj.class_p = &world_p->bool_class;
  world_p->false_obj.rc = 1;

  world_p->true_obj.class_p = &world_p->bool_class;
  world_p->true_obj.rc = 1;

  world_p->core_module.obj.class_p = &world_p->module_class;
  world_p->core_module.obj.rc = 1;

  ox = T40_NREF_INDEX(T40_ID_FSD(world_p, "topor.v00.core"));
  if (!ox) { LE("failed to create core id"); goto l_error; }
  world_p->core_module.id_ox = ox;

  ox = T40_NREF_INDEX(T40_ID_FSD(world_p, "object"));
  if (!ox) { LE("failed to create 'object' id"); goto l_error; }
  world_p->object_class.id_ox = ox;

  ox = T40_NREF_INDEX(T40_ID_FSD(world_p, "class"));
  if (!ox) { LE("failed to create 'class' id"); goto l_error; }
  world_p->class_class.id_ox = ox;

  ox = T40_NREF_INDEX(T40_ID_FSD(world_p, "module"));
  if (!ox) { LE("failed to create 'module' id"); goto l_error; }
  world_p->module_class.id_ox = ox;

  ox = T40_NREF_INDEX(T40_ID_FSD(world_p, "func"));
  if (!ox) { LE("failed to create 'func' id"); goto l_error; }
  world_p->func_class.id_ox = ox;

  ox = T40_NREF_INDEX(T40_ID_FSD(world_p, "null"));
  if (!ox) { LE("failed to create 'null' id"); goto l_error; }
  world_p->null_class.id_ox = ox;

  ox = T40_NREF_INDEX(T40_ID_FSD(world_p, "object"));
  if (!ox) { LE("failed to create 'object' id"); goto l_error; }
  world_p->object_class.id_ox = ox;

  ox = T40_NREF_INDEX(T40_ID_FSD(world_p, "cba"));
  if (!ox) { LE("failed to create 'cba' id"); goto l_error; }
  world_p->cba_class.id_ox = ox;

  ox = T40_NREF_INDEX(T40_ID_FSD(world_p, "ba"));
  if (!ox) { LE("failed to create 'ba' id"); goto l_error; }
  world_p->ba_class.id_ox = ox;

  ox = T40_NREF_INDEX(T40_ID_FSD(world_p, "id"));
  if (!ox) { LE("failed to create 'id' id"); goto l_error; }
  world_p->id_class.id_ox = ox;

  ox = T40_NREF_INDEX(T40_ID_FSD(world_p, "bool"));
  if (!ox) { LE("failed to create 'bool' id"); goto l_error; }
  world_p->bool_class.id_ox = ox;

  ox = T40_NREF_INDEX(T40_ID_FSD(world_p, "int"));
  if (!ox) { LE("failed to create 'int' id"); goto l_error; }
  world_p->ref_int_class.id_ox = ox;

  rc = t40_set_field(world_p, OX_TO_REF(T40_OX_CORE),
                     OX_TO_REF(world_p->object_class.id_ox), OX_TO_REF(T40_OX_OBJECT_CLASS));
  if (rc) { LE("ouch"); goto l_error; }

  rc = t40_set_field(world_p, OX_TO_REF(T40_OX_CORE),
                     OX_TO_REF(world_p->class_class.id_ox), OX_TO_REF(T40_OX_CLASS_CLASS));
  if (rc) { LE("ouch"); goto l_error; }

  return T40_OK;

l_ma_error:
  world_p->ma_rc = ma_rc;
  rc = T40_MEM_ERROR;
l_error:
  t40_world_finish(world_p);
  return rc;
}

/* t40_world_finish *********************************************************/
T40_API uint_t C41_CALL t40_world_finish (t40_world_t * world_p)
{
  uint_t c;
  c41_np_t * np_p;
  t40_flow_t * flow_p;

  LI("finishing...");
  world_p->is_ending = 1;
  for (np_p = world_p->flow_list.next; np_p != &world_p->flow_list;)
  {
    flow_p = T40_FLOW_FROM_LINKS(np_p);
    np_p = np_p->next;

    c = C41_VAR_FREE1(&world_p->ma, flow_p);
  }

  if (world_p->obj.pa)
  {
    uint_t ox, nf_ox;

    /* go through the chain of free indexes and clear the pointers
     * so that we can destroy all non-NULL object pointers from the table */
    for (ox = world_p->ff_ox; ox; ox = nf_ox)
    {
      LI("nullifying object pointer for ox=$Xd", ox);
      nf_ox = world_p->obj.nf_oxa[ox];
      world_p->obj.pa[ox] = NULL;
    }
    /* delete fields & clear ref counts for all objects */
    for (ox = 0; ox < world_p->uninit_ox; ++ox)
    {
      t40_obj_t * obj_p = world_p->obj.pa[ox];
      if (!obj_p) continue;
      if (obj_p->field_n)
      {
        uint_t i;
        LI("clearing fields: ox=$Xd, fn=$Xd", ox, obj_p->field_n);
        for (i = 0; i < obj_p->field_n; ++i)
        {
          c = t40_deref(world_p, obj_p->field_a[i].key);
          if (c) { LE("ouch"); return c; }
          c = t40_deref(world_p, obj_p->field_a[i].value);
          if (c) { LE("ouch"); return c; }
        }
        c = C41_VAR_FREE(&world_p->ma, obj_p->field_a, obj_p->field_n);
        if (c) { LE("ouch"); world_p->ma_rc = c; return T40_MEM_ERROR; }
      }

      LI("clearing ref counter for ox=$Xd (rc=$Xd)",
           ox, world_p->obj.pa[ox]->rc);
      world_p->obj.pa[ox]->rc = 0;
    }

    /* compute finish dependencies between objects */
    for (ox = 0; ox < world_p->uninit_ox; ++ox)
    {
      t40_obj_t * obj_p;
      t40_class_t * class_p;
      uint_t c;
      obj_p = world_p->obj.pa[ox];
      if (!obj_p) continue;
      class_p = obj_p->class_p;
      LI("marking finish deps for ox=$Xd", ox);
      c = class_p->mark_finish_deps(world_p, obj_p);
      if (c)
      {
        LE("failed mark_finish_deps for object #$Xd ($s = $Xi)",
           ox, t40_status_name(c), c);
        return world_p->finish_rc = c;
      }
    }

    /* now start destroying objects with no dependencies, which
     * should trigger eventually the destruction of those with dependencies
     * when the references to them are removed */
    for (ox = 0; ox < world_p->uninit_ox; ++ox)
    {
      t40_obj_t * obj_p;
      t40_class_t * class_p;
      obj_p = world_p->obj.pa[ox];
      if (!obj_p || obj_p->rc) continue;
      class_p = obj_p->class_p;
      LI("finishing object: ox=$Xd", ox);
      c = class_p->finish(world_p, obj_p);
      if (c)
      {
        LE("failed finalising object $Xd ($s = $Xi)",
           ox, t40_status_name(c), c);
        return world_p->finish_rc = T40_FAILED;
      }
      LI("freeing memory: ox=$Xd, size=$Xz", ox, class_p->instance_size);
      if (ox >= T40_OX__COUNT)
      {
        c = c41_ma_free(&world_p->ma, obj_p, class_p->instance_size);
        if (c)
        {
          world_p->ma_rc = c;
          return world_p->finish_rc = T40_MEM_ERROR;
        }
      }
    }

    LI("freeing object table...");
    c = C41_VAR_FREE(&world_p->ma, world_p->obj.pa, world_p->lim_ox);
    if (c)
    {
      world_p->ma_rc = c;
      return world_p->finish_rc = T40_FAILED;
    }
  }

  return world_p->finish_rc = T40_OK;
}

/* ox_alloc *****************************************************************/
C41_LOCAL t40_ox_t C41_CALL ox_alloc (t40_world_t * world_p)
{
  t40_ox_t ox;
  if (!world_p->ff_ox)
  {
    if (world_p->uninit_ox == world_p->lim_ox)
    {
      LE("TODO: realloc table of object pointers");
      world_p->lrc = T40_NO_CODE;
      return 0;
    }
    /* here we have uninit_ox < lim_ox */
    A(world_p->uninit_ox < world_p->lim_ox);
    ox = world_p->uninit_ox++;
    //return ox;
  }
  else
  {
    ox = world_p->ff_ox;
    world_p->ff_ox = (t40_ox_t) world_p->obj.nf_oxa[ox];
  }
  // LI("ox=$d. ff_ox=$d, uninit_ox=$d, lim_ox=$d",
  //    ox, world_p->ff_ox, world_p->uninit_ox, world_p->lim_ox);
  return ox;

#if _DEBUG
l_bug:
  world_p->lrc = T40_BUG;
  return 0;
#endif
}

/* ox_free ******************************************************************/
C41_LOCAL void C41_CALL ox_free (t40_world_t * world_p, t40_ox_t ox)
{
  if (world_p->is_ending) return;
  world_p->obj.nf_oxa[ox] = world_p->ff_ox;
  world_p->ff_ox = ox;
  LI("ff_ox=$d, uninit_ox=$d, lim_ox=$d",
     world_p->ff_ox, world_p->uninit_ox, world_p->lim_ox);
}

/* obj_alloc ****************************************************************/
C41_LOCAL void * C41_CALL obj_alloc
(
  t40_world_t * world_p,
  t40_class_t * class_p
)
{
  uint_t ma_rc;
  uint_t rc;
  t40_obj_t * obj_p;

  rc = ox_ref(world_p, class_p->self_ox);
  if (rc)
  {
    LE("failed to add class reference while allocating an object");
    world_p->lrc = rc;
    return NULL;
  }

  ma_rc = c41_ma_alloc(&world_p->ma, (void * *) &obj_p, class_p->instance_size);
  if (ma_rc)
  {
    world_p->ma_rc = ma_rc;
    world_p->lrc = T40_MEM_ERROR;
    LE("no mem for object");
    return NULL;
  }
  obj_p->class_p = class_p;
  obj_p->field_a = NULL;
  obj_p->field_n = 0;
  obj_p->rc = 1;
  return obj_p;
}

/* obj_create ***************************************************************/
C41_LOCAL t40_ox_t C41_CALL obj_create
(
  t40_world_t * world_p,
  t40_class_t * class_p
)
{
  t40_obj_t * obj_p;
  t40_ox_t ox;

  ox = ox_alloc(world_p);
  if (!ox) return 0;
  obj_p = obj_alloc(world_p, class_p);
  if (!obj_p)
  {
    ox_free(world_p, ox);
    return 0;
  }
  //LI("obj_create: ox=$Xd p=$G4p z=$Xd", ox, obj_p, class_p->instance_size);
  world_p->obj.pa[ox] = obj_p;
  return ox;
}

/* t40_cba_static ***********************************************************/
T40_API t40_ref_t C41_CALL t40_cba_static
(
  t40_world_t * world_p,
  uint8_t const * data_a,
  size_t data_n
)
{
  t40_ox_t ox;
  t40_ba_t * ba_p;

  ox = obj_create(world_p, &world_p->cba_class);
  if (!ox) return 0;

  ba_p = world_p->obj.vpa[ox];
  ba_p->a = (uint8_t *) data_a;
  ba_p->n = data_n;
  ba_p->m = data_n;
  ba_p->allocated = 0;
  ba_p->writable = 0;
  ba_p->resizeable = 0;
  return OX_TO_REF(ox);
}

/* t40_cba_static_str *******************************************************/
T40_API t40_ref_t C41_CALL t40_cba_static_cstr
(
  t40_world_t * world_p,
  char const * data_azt // NUL-ended string
)
{
  size_t data_n = C41_STR_LEN(data_azt);
  return t40_cba_static(world_p, (uint8_t const *) data_azt, data_n);
}

/* class_init ***************************************************************/
C41_LOCAL uint_t C41_CALL class_init
(
  t40_world_t * world_p,
  t40_ox_t self_ox,
  t40_ox_t super_ox,
  size_t instance_size,
  t40_action_f finish,
  t40_action_f mark_finish_deps
)
{
  t40_class_t * class_p = (t40_class_t *) world_p->obj.pa[self_ox];
  uint_t rc;

  class_p->obj.class_p = &world_p->class_class;
  class_p->obj.field_a = NULL;
  class_p->obj.field_n = 0;
  class_p->obj.rc = 1;
  class_p->finish = finish;
  class_p->mark_finish_deps = mark_finish_deps;
  class_p->instance_size = instance_size;
  class_p->method_a = NULL;
  class_p->method_n = 0;
  class_p->self_ox = self_ox;
  class_p->super_ox = super_ox;

  rc = ox_ref(world_p, super_ox);

  return rc;
}

/* t40_id_fsd ***************************************************************/
T40_API t40_ref_t C41_CALL t40_id_fsd
(
  t40_world_t * world_p,
  uint8_t const * data_a,
  size_t data_n
)
{
  c41_rbtree_path_t path;
  c41_u8an_t data_an;
  t40_id_t * id_p;
  int fstat;
  t40_ox_t ox, cba_ox;
  t40_ref_t cba_r;
  uint_t c;

  data_an.a = (uint8_t *) data_a;
  data_an.n = data_n;
  fstat = c41_rbtree_find(&path, &world_p->id_cba_tree, &data_an);
  if (!fstat)
  {
    // found key
    id_p = C41_FIELD_TO_OBJECT(t40_id_t, rbn, path.nodes[path.last]);
    return OX_TO_REF(id_p->self_ox);
  }

  cba_r = t40_cba_static(world_p, data_a, data_n);
  if (!cba_r) return 0;
  cba_ox = T40_NREF_INDEX(cba_r);

  ox = obj_create(world_p, &world_p->id_class);
  if (!ox)
  {
    c = ox_deref(world_p, cba_ox);
    if (c) world_p->lrc = c;
    return 0;
  }
  id_p = world_p->obj.vpa[ox];
  id_p->cba_ox = cba_ox;

  id_p->self_ox = ox;
  c41_rbtree_insert(&path, &id_p->rbn);

  return OX_TO_REF(ox);
}

/* ox_deref *****************************************************************/
C41_LOCAL uint_t C41_CALL ox_deref (t40_world_t * world_p, t40_ox_t ox)
{
  t40_obj_t * obj_p;
  uint_t rc;
  t40_class_t * class_p;
  obj_p = world_p->obj.pa[ox];
  if (!obj_p) return 0; // object already destroyed - can happen during gc
  if (--(obj_p->rc)) return 0;
  LI("destroying object: ox=$Xd...", ox);
  class_p = obj_p->class_p;
  rc = class_p->finish(world_p, obj_p);
  if (rc) return rc;
  LI("freeing object data: ox=$Xd...", ox);
  rc = c41_ma_free(&world_p->ma, obj_p, class_p->instance_size);
  if (rc)
  {
    world_p->ma_rc = rc;
    return T40_MEM_ERROR;
  }
  ox_free(world_p, ox);
  return 0;
}

/* t40_ref ******************************************************************/
T40_API uint_t C41_CALL t40_ref
(
  t40_world_t * world_p,
  t40_ref_t obj_r
)
{
  if (!T40_IS_NREF(obj_r)) return 0;
  return ox_ref(world_p, T40_NREF_INDEX(obj_r));
}

/* t40_deref ****************************************************************/
T40_API uint_t C41_CALL t40_deref
(
  t40_world_t * world_p,
  t40_ref_t obj_r
)
{
  if (!T40_IS_NREF(obj_r)) return 0;
  return ox_deref(world_p, T40_NREF_INDEX(obj_r));
}

/* t40_set_field ************************************************************/
T40_API uint_t C41_CALL t40_set_field (t40_world_t * world_p, t40_ref_t obj_r,
                                       t40_ref_t field_id_r, t40_ref_t value_r)
{
  t40_ox_t ox;
  t40_obj_t * obj_p;
  int32_t a, b, c;
  t40_ref_t key_r;
  uint_t rc;

  if (!T40_IS_NREF(obj_r)) return T40_NO_FIELDS;

  ox = T40_NREF_INDEX(obj_r);
  A(ox < world_p->uninit_ox);
  obj_p = world_p->obj.pa[ox];

  for (a = 0, b = obj_p->field_n - 1; a <= b; )
  {
    c = (a + b) >> 1;
    key_r = obj_p->field_a[c].key;
    if (key_r == field_id_r)
    {
      // replace old value with the new one
      rc = t40_deref(world_p, obj_p->field_a[c].value);
      if (rc) return rc;
      rc = t40_ref(world_p, value_r);
      if (rc) return rc;
      obj_p->field_a[c].value = value_r;
      return 0;
    }
    if (field_id_r < key_r) b = c - 1;
    else a = c + 1;
  }
  if (obj_p->field_n == FIELD_LIM) return T40_FIELD_LIM;

  if ((obj_p->field_n & (FIELD_GRAN - 1)) == 0)
  {
    LI("realloc field table for ox=$Xd to n=$Xd", ox, obj_p->field_n + FIELD_GRAN);
    rc = C41_VAR_REALLOC(&world_p->ma, obj_p->field_a,
                         obj_p->field_n + FIELD_GRAN, obj_p->field_n);
    if (rc)
    {
      world_p->ma_rc = rc;
      return T40_MEM_ERROR;
    }
  }
  for (c = obj_p->field_n; c > a; --c)
    obj_p->field_a[c] = obj_p->field_a[c - 1];

  rc = t40_ref(world_p, field_id_r);
  if (rc) return rc;
  rc = t40_ref(world_p, value_r);
  if (rc) return rc;
  obj_p->field_a[a].key = field_id_r;
  obj_p->field_a[a].value = value_r;
  obj_p->field_n += 1;
  return 0;
  goto l_bug;
l_bug:
  return T40_BUG;
}

