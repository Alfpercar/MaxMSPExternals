#include "ext.h"

/* ---- buddy.c --- data flow management object 
		updated 5/18/92 ddz to use proxies,
		event num synchonization, and any
		number of inlets

-- */

typedef struct {
	void *m_proxy;
	void *m_out;
	short m_argc;
	t_atom m_argv[128];
	short m_on;
} t_member;

typedef struct {
	t_object b_ob;
	long b_num;
	t_member *b_mem;
	long b_id;
} t_buddy;

void buddy_bang(t_buddy *x);
void buddy_anything(t_buddy *x, t_symbol *s, short argc, t_atom *argv);
void buddy_float(t_buddy *x,double f);
void buddy_int(t_buddy *x,long n);
void buddy_list(t_buddy *x, t_symbol *s, short argc, t_atom *argv);
void buddy_atom(t_buddy *x, t_atom *a);
short buddy_all(t_buddy *x);
void buddy_off(t_buddy *x);
void buddy_out(t_buddy *x);
void outlet_member(void *out, short argc, t_atom *argv);
void buddy_assist(t_buddy *x, void *b, long m, long a, char *s);
void buddy_inletinfo(t_buddy *x, void *b, long a, char *t);
void buddy_free(t_buddy *x);
void *buddy_new(long num);
void buddy_clear(t_buddy *x);

void *buddy_class;
t_symbol *ps_list;

int main()
{
	setup((t_messlist **)&buddy_class, (method)buddy_new, (method)buddy_free, (short)sizeof(t_buddy), 0L, A_DEFLONG, 0);
	addint((method)buddy_int);
	addbang((method)buddy_bang);
	addfloat((method)buddy_float);
	addmess((method)buddy_list, "list", A_GIMME, 0);
	addmess((method)buddy_clear, "clear", 0);
	addmess((method)buddy_anything, "anything", A_GIMME, 0);
	addmess((method)buddy_assist,"assist",A_CANT,0);
	addmess((method)buddy_inletinfo,"inletinfo",A_CANT,0);
	finder_addclass("Right-to-Left","buddy");
	ps_list = gensym("list");			

	return 0;
}

void buddy_bang(t_buddy *x)
{
	buddy_int(x,0L);
}

void buddy_clear(t_buddy *x)
{
	buddy_off(x);
}

void buddy_anything(t_buddy *x, t_symbol *s, short argc, t_atom *argv)
{
	t_member *m;
	long in = proxy_getinlet((t_object *)x);
	
	m = x->b_mem + in;
	m->m_on = TRUE;
	m->m_argc = argc + 1;
	m->m_argv[0].a_type = A_SYM;
	m->m_argv[0].a_w.w_sym = s;
	if (argc > 127)
		argc = 127;
	sysmem_copyptr(argv,m->m_argv+1,argc * sizeof(t_atom));
	if (buddy_all(x)) {
		buddy_off(x);
		buddy_out(x);
	}
}

void buddy_list(t_buddy *x, t_symbol *s, short argc, t_atom *argv)
{
	buddy_anything(x,ps_list,argc,argv);
}

void buddy_float(t_buddy *x, double f)
{
	t_atom a;
	
	a.a_type = A_FLOAT;
	a.a_w.w_float = f;
	buddy_atom(x,&a);
}

void buddy_int(t_buddy *x, long n)
{
	t_atom a;
	
	a.a_type = A_LONG;
	a.a_w.w_long = n;
	buddy_atom(x,&a);
}

void buddy_atom(t_buddy *x, t_atom *a)
{
	t_member *m;
	long in = proxy_getinlet((t_object *)x);
	
	m = x->b_mem + in;
	m->m_on = true;
	m->m_argc = 1;
	m->m_argv[0] = *a;
	if (buddy_all(x)) {
		buddy_off(x);
		buddy_out(x);
	}
}

short buddy_all(t_buddy *x)
{
	short i;
	t_member *m;
	
	for (i=0,m = x->b_mem; i < x->b_num; i++,m++)
		if (!m->m_on)
			return false;
	return true;
}

void buddy_off(t_buddy *x)
{
	short i;
	t_member *m;
	
	for (i=0,m = x->b_mem; i < x->b_num; i++,m++)
		m->m_on = 0;
}

void buddy_out(t_buddy *x)
{
	short i;
	t_member *m;
	
	for (i=x->b_num-1,m = x->b_mem+i; i >= 0; i--,m--)
		outlet_member(m->m_out,m->m_argc,m->m_argv);
}

void outlet_member(void *out, short argc, t_atom *argv)
{
	if (argc == 1) {
		switch (argv->a_type) {
			case A_LONG:
				outlet_int(out,argv->a_w.w_long);
				break;
			case A_FLOAT:
				outlet_float(out,argv->a_w.w_float);
				break;
			case A_SYM:
				outlet_anything(out,argv->a_w.w_sym,0,0L);
				break;
		}
	} else if (argv->a_w.w_sym == ps_list)
		outlet_list(out,ps_list,argc-1,argv+1);
	else
		outlet_anything(out,argv->a_w.w_sym,argc-1,argv+1);
}

void buddy_assist(t_buddy *x, void *b, long m, long a, char *s)
{
	if (m == ASSIST_INLET)
		sprintf(s,"Input to be Synchronized");
	else 
		sprintf(s,"Synchronized Output of Inlet %ld", a+1);
}

void buddy_inletinfo(t_buddy *x, void *b, long a, char *t)
{
	/*
	 *	red if every  input - 1 has been set and you're looking to the one which will trigger
	 */
	long i, count;
	t_member *m, *m2;
	
	for (i = count = 0, m = x->b_mem; i < x->b_num; i++, m++) {
		if (i == a)
			m2 = m;
		if (m->m_on)
			count++;
	}
	
	if (count >= (x->b_num - 1) && ! m2->m_on)
		*t = 0;
	else
		*t = 1;
}

void buddy_free(t_buddy *x)
{
	short i;
	
	for (i=1; i < x->b_num; i++)
		freeobject(x->b_mem[i].m_proxy);
	freebytes(x->b_mem,(unsigned short)(x->b_num * sizeof(t_member)));
}

void *buddy_new(long num)
{
	t_buddy *x;
	short i;
	t_member *m;

	x = (t_buddy *)newobject(buddy_class);
	if (num < 2)
		num = 2;
	x->b_num = num;
	x->b_id = 0;
	x->b_mem = (t_member *)getbytes((unsigned short)(num * sizeof(t_member)));
	for (i=num-1,m = x->b_mem + i; i >= 0; i--,m--) {
		if (i)
			m->m_proxy = proxy_new(x,(long)i,&x->b_id);
		m->m_out = outlet_new(x,0L);
		m->m_on = 0;
		m->m_argc = 1;
		m->m_argv[0].a_type = A_LONG;
		m->m_argv[0].a_w.w_long = 0;
	}
	return x;
}

