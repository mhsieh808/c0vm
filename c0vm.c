#include <assert.h>
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>


#include "lib/xalloc.h"
#include "lib/stack.h"
#include "lib/contracts.h"
#include "lib/c0v_stack.h"
#include "lib/c0vm.h"
#include "lib/c0vm_c0ffi.h"
#include "lib/c0vm_abort.h"

/* call stack frames */
typedef struct frame_info frame;
struct frame_info {
  c0v_stack_t S; /* Operand stack of C0 values */
  ubyte *P;      /* Function body */
  size_t pc;     /* Program counter */
  c0_value *V;   /* The local variables */
};

int execute(struct bc0_file *bc0) {
  REQUIRES(bc0 != NULL);

  /* Variables */
  c0v_stack_t S=c0v_stack_new(); /* Operand stack of C0 values */
  ubyte *P=((bc0->function_pool)[0]).code;      /* Array of bytes that make up the current function */
  size_t pc=0;     /* Current location within the current byte array P */
  c0_value *V=xcalloc(sizeof(c0_value), bc0->function_pool->num_vars);   /* Local variables (you won't need this till Task 2) */
  //(void) V;

  /* The call stack, a generic stack that should contain pointers to frames */
  /* You won't need this until you implement functions. */
  gstack_t callStack=stack_new();
  //(void) callStack;

  while (true) {

// #ifdef DEBUG
//     /* You can add extra debugging information here */
//     fprintf(stderr, "Opcode %x -- Stack size: %zu -- PC: %zu\n",
//             P[pc], c0v_stack_size(S), pc);
// #endif

    switch (P[pc]) {

    /* Additional stack operation: */

    case POP: {
      pc++;
      c0v_pop(S);
      break;
    }

    case DUP: {
      pc++;
      c0_value v = c0v_pop(S);
      c0v_push(S,v);
      c0v_push(S,v);
      break;
    }

    case SWAP: {
      pc++;
      c0_value v1=c0v_pop(S);
      c0_value v2=c0v_pop(S);
      c0v_push(S, v1);
      c0v_push(S, v2);
      break;
    }


    /* Returning from a function.
     * This currently has a memory leak! You will need to make a slight
     * change for the initial tasks to avoid leaking memory.  You will
     * need to be revise it further when you write INVOKESTATIC. */

    case RETURN: {
      c0_value retval=c0v_pop(S);
      assert(c0v_stack_empty(S));
      c0v_stack_free(S);
      free(V);
      if (stack_empty(callStack))
      {
        stack_free(callStack, &free);
        return val2int(retval);
      }
      frame *F=(frame*)pop(callStack);
      V=F->V;
      P=F->P;
      pc=F->pc;
      S=F->S;
      c0v_push(S, retval);
      free(F);
      break;

// #ifdef DEBUG
//       fprintf(stderr, "Returning %d from execute()\n", retval);
// #endif
      // Free everything before returning from the execute function!
    }


    /* Arithmetic and Logical operations task 1*/

    case IADD:
    {
      pc++;
      int v1=val2int(c0v_pop(S));
      int v2=val2int(c0v_pop(S));
      c0v_push(S, int2val(v1+v2));
      break;
    }

    case ISUB:
    {
      pc++;
      int v1=val2int(c0v_pop(S));
      int v2=val2int(c0v_pop(S));
      c0v_push(S, int2val(v2-v1));
      break;
    }

    case IMUL:
    {
      pc++;
      int v1=val2int(c0v_pop(S));
      int v2=val2int(c0v_pop(S));
      c0v_push(S, int2val(v1*v2));
      break;
    }

    case IDIV:
    {
      pc++;
      int v1=val2int(c0v_pop(S));
      int v2=val2int(c0v_pop(S));
      if (v1==0) c0_arith_error("Division by 0 error\n");
      if (v2==INT_MIN && v1==-1) c0_arith_error("Division of INT_MIN by -1 error\n");
      c0v_push(S, int2val(v2/v1));
      break;
    }

    case IREM:
    {
      pc++;
      int v1=val2int(c0v_pop(S));
      int v2=val2int(c0v_pop(S));
      if (v1==0) c0_arith_error("Mod by 0 error\n");
      if (v2==INT_MIN && v1==-1) c0_arith_error("Mod of INT_MIN by -1 error\n");
      c0v_push(S, int2val(v2%v1));
      break;
    }

    case IAND:
    {
      pc++;
      int v1=val2int(c0v_pop(S));
      int v2=val2int(c0v_pop(S));
      c0v_push(S, int2val(v1&v2));
      break;
    }

    case IOR:
    {
      pc++;
      int v1=val2int(c0v_pop(S));
      int v2=val2int(c0v_pop(S));
      c0v_push(S, int2val(v2|v1));
      break;
    }

    case IXOR:
    {
      pc++;
      int v1=val2int(c0v_pop(S));
      int v2=val2int(c0v_pop(S));
      c0v_push(S, int2val(v2^v1));
      break;
    }

    case ISHL:
    {
      pc++;
      int v1=val2int(c0v_pop(S));
      int v2=val2int(c0v_pop(S));
      if (v1<0) c0_arith_error("Cannot shift by negative value\n");
      if (v1>31) c0_arith_error("Cannot shift by a value greater than 32\n");
      c0v_push(S, int2val(v2<<v1));
      break;
    }

    case ISHR:
    {
      pc++;
      int v1=val2int(c0v_pop(S));
      int v2=val2int(c0v_pop(S));
      if (v1<0) c0_arith_error("Cannot shift by negative value\n");
      if (v1>31) c0_arith_error("Cannot shift by a value greater than 32\n");
      c0v_push(S, int2val(v2>>v1));
      break;
    }


    /* Pushing constants task 1 and 2*/

    case BIPUSH:
    {
      pc++;
      int v=(int)(signed char)(P[pc]);
      pc++;
      c0v_push(S, int2val(v));
      break;
    }

    case ILDC:
    {
      pc++;
      unsigned int v1=(unsigned int)P[pc];
      pc++;
      unsigned int v2=(unsigned int)P[pc];
      pc++;
      unsigned int i=((v1<<8)|v2);
      if (i>bc0->int_count) c0_memory_error("ILDC error\n");
      c0v_push(S, int2val(bc0->int_pool[i]));
      break;
    }

    case ALDC:
    {
      pc++;
      unsigned int v1=(unsigned int)P[pc];
      pc++;
      unsigned int v2=(unsigned int)P[pc];
      pc++;
      unsigned i=((v1<<8)|v2);
      if (i>bc0->string_count) c0_memory_error("ALDC error\n");
      c0v_push(S, ptr2val(&bc0->string_pool[i]));
      break;

    }

    case ACONST_NULL:
    {
      pc++;
      c0v_push(S, ptr2val(NULL));
      break;
    }


    /* Operations on local variables task 2*/

    case VLOAD:
    {
      pc++;
      ubyte i=P[pc];
      c0v_push(S, V[i]);
      pc++;
      break;

    }

    case VSTORE:
    {
      pc++;
      ubyte i=P[pc];
      V[i]=c0v_pop(S);
      pc++;
      break;
    }


    /* Control flow operations task 3*/

    case NOP:
    {
      pc++;
      break;
    }

    case IF_CMPEQ:
    {
      pc++;
      ubyte v1=P[pc];
      pc++;
      ubyte v2=P[pc];
      pc++;
      signed short offset=(signed short)((v1<<8)|v2); 
      if (val_equal(c0v_pop(S), c0v_pop(S))) pc=pc-3+offset;
      break;
    }

    case IF_CMPNE:
    {
      pc++;
      ubyte v1=P[pc];
      pc++;
      ubyte v2=P[pc];
      pc++;
      signed short offset=(signed short)((v1<<8)|v2);
      if (!val_equal(c0v_pop(S), c0v_pop(S))) pc=pc-3+offset;
      break;
    }

    case IF_ICMPLT:
    {
      pc++;
      ubyte v1=P[pc];
      pc++;
      ubyte v2=P[pc];
      pc++;
      signed short offset=(signed short)((v1<<8)|v2);
      if (val2int(c0v_pop(S))>val2int(c0v_pop(S))) pc=pc-3+offset;
      break;
    }

    case IF_ICMPGE:
    {
      pc++;
      ubyte v1=P[pc];
      pc++;
      ubyte v2=P[pc];
      pc++;
      signed short offset=(signed short)((v1<<8|v2));
      if (val2int(c0v_pop(S))<=val2int(c0v_pop(S))) pc=pc-3+offset;
      break;
    }

    case IF_ICMPGT:
    {
      pc++;
      ubyte v1=P[pc];
      pc++;
      ubyte v2=P[pc];
      pc++;
      signed short offset=(signed short)((v1<<8|v2));
      if (val2int(c0v_pop(S))<val2int(c0v_pop(S))) pc=pc-3+offset;
      break;
    }

    case IF_ICMPLE:
    {
      pc++;
      ubyte v1=P[pc];
      pc++;
      ubyte v2=P[pc];
      pc++;
      signed short offset=(signed short)((v1<<8)|v2);
      if (val2int(c0v_pop(S))>=val2int(c0v_pop(S))) pc=pc-3+offset;
      break;
    }


    case GOTO:
    {
      pc++;
      ubyte v1=P[pc];
      pc++;
      ubyte v2=P[pc];
      pc++;
      signed short offset=(signed short)((v1<<8)|v2);
      pc=pc-3+offset;
      break;
    }

    case ATHROW:
    {
      pc++;
      char* c=val2ptr(c0v_pop(S));
      c0_user_error(c);
      break;
    }

    case ASSERT:
    {
      pc++;
      char* c=val2ptr(c0v_pop(S));
      int i=val2int(c0v_pop(S));
      if (i==0) c0_assertion_failure(c);
      break;
    }


    /* Function call operations: task 4 */

    case INVOKESTATIC:
    {
      pc++;
      unsigned short v1=(unsigned short)(unsigned char)P[pc];
      pc++;
      unsigned short v2=(unsigned short)(unsigned char)P[pc];
      pc++;
      unsigned short i=((v1<<8)|v2);
      frame *nf=xmalloc(sizeof(frame));
      nf->V=V;
      nf->P=P;
      nf->pc=pc;
      nf->S=S;
      push(callStack, nf);
      struct function_info *fi=&bc0->function_pool[i];
      V=xcalloc(fi->num_vars, sizeof(c0_value));
      for (int j=fi->num_args-1; j>=0;j--) V[j]=c0v_pop(S);
      S=c0v_stack_new();
      P=fi->code;
      pc=0;
      break;
    }

    case INVOKENATIVE:
    {
      pc++;
      unsigned short v1=(unsigned short)(unsigned char)P[pc];
      pc++;
      unsigned short v2=(unsigned short)(unsigned char)P[pc];
      pc++;
      unsigned short i=((v1<<8)|v2);
      struct native_info *ni=&bc0->native_pool[i];
      c0_value *nv=xcalloc(sizeof(c0_value), ni->num_args);
      for (int j=ni->num_args-1; j>=0; j--) nv[j]=c0v_pop(S);
      c0_value fv=(*native_function_table[ni->function_table_index])(nv);
      c0v_push(S, fv);
      free(nv);
      break;
    }



    /* Memory allocation operations: task 5*/

    case NEW:
    {
      pc++;
      ubyte s=P[pc];
      void *b=xcalloc(1, s);
      c0v_push(S, ptr2val(b));
      pc++;
      break;

    }

    case NEWARRAY:
    {
      pc++;
      ubyte size=P[pc];
      int n=val2int(c0v_pop(S));
      if (n<0) c0_memory_error("Invalid size");
      c0_array *B=xmalloc(sizeof(c0_array));
      B->count=n;
      B->elt_size=size;
      B->elems=xcalloc(n, size);
      c0v_push(S, ptr2val((void*)B));
      pc++;
      break;
    }

    case ARRAYLENGTH:
    {
      pc++;
      c0_array *B=(c0_array*)val2ptr(c0v_pop(S));
      if (B==NULL) c0_memory_error("Dereferencing NULL");
      c0v_push(S, int2val(B->count));
      break;
    }


    /* Memory access operations: task 5*/

    case AADDF:
    {
      pc++;
      ubyte f=P[pc];
      char *p=(char*)val2ptr(c0v_pop(S));
      if (p==NULL) c0_memory_error("AADF error");
      c0v_push(S, ptr2val((void*)&p[f]));
      pc++;
      break;

    }

    case AADDS:
    {
      pc++;
      int i=val2int(c0v_pop(S));
      c0_array *arr=(c0_array*)val2ptr(c0v_pop(S));
      if (arr==NULL) c0_memory_error("AADDS error");
      if (i<0) c0_memory_error("AADDS error");
      if (i>=arr->count) c0_memory_error("AADDS error");
      char *e=(char*)arr->elems;
      c0v_push(S, ptr2val((void*)(&e[i*arr->elt_size])));
      break;


    }

    case IMLOAD:
    {
      pc++;
      int *a=(int*)val2ptr(c0v_pop(S));
      if (a==NULL) c0_memory_error("IMLOAD error");
      c0v_push(S, int2val(*a));
      break;

    }

    case IMSTORE:
    {
      pc++;
      int x=val2int(c0v_pop(S));
      int *a=(int*)val2ptr(c0v_pop(S));
      if (a==NULL) c0_memory_error("IMSTORE error");
      *a=x;
      break;

    }

    case AMLOAD:
    {
      pc++;
      void **a=val2ptr(c0v_pop(S));
      if (a==NULL) c0_memory_error("AMLOAD error");
      void *b=*a;
      c0v_push(S, ptr2val(b));
      break;

    }

    case AMSTORE:
    {
      pc++;
      void *a=val2ptr(c0v_pop(S));
      void **b=val2ptr(c0v_pop(S));
      if (b==NULL) c0_memory_error("AMSTORE error");
      *b=a;
      break;

    }

    case CMLOAD:
    {
      pc++;
      char *a=(char*)val2ptr(c0v_pop(S));
      if (a==NULL) c0_memory_error("CMLOAD error");
      c0v_push(S, int2val((int)*a));
      break;
    }

    case CMSTORE:
    {
      pc++;
      int a=val2int(c0v_pop(S));
      char *b=(char*)val2ptr(c0v_pop(S));
      if (b==NULL) c0_memory_error("CMSTORE error");
      *b=(a & 0x7f);
      break;

    }

    default:
      fprintf(stderr, "invalid opcode: 0x%02x\n", P[pc]);
      abort();
    }
  }

  /* cannot get here from infinite loop */
  assert(false);
}
