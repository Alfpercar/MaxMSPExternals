/* match.c -- triggering based on sequential input ------- */

#include "ext.h"
#include "ext_critical.h"

#define ANYONE MAGIC
#define NOVALUE 0x80000000

typedef struct match
{
	t_object m_ob;
	t_atom *m_seen,*m_want;
	short m_size;
	short m_where;
	void *m_out;
	t_critical m_critical;
} t_match;

void *match_class;

void match_assist(t_match *x, void *b, long m, long a, char *s);
void match_freebytes(t_match *x);
void match_int(t_match *x, long n);
void match_float(t_match *x, double f);
void match_list(t_match *x, t_symbol *s, short argc, t_atom *argv);
void match_anything(t_match *x, t_symbol *s, short argc, t_atom *argv);
void match_atom(t_match *x, t_atom *a);
void atom_compare(t_match *x);
Boolean atom_equal(t_atom *a, t_atom *b);
void outlet_atomlist(void *out, long argc, t_atom *argv);
void match_clear(t_match *x);
void match_set(t_match *x, t_symbol *s, short ac, t_atom *av);
void match_setwant(t_atom *temp, short ac, t_atom *av);
void match_free(t_match *x);
void *match_new(t_symbol *s, short ac, t_atom *av);

t_atom atom_novalue = { A_LONG, {NOVALUE}  };
t_symbol *ps_nn, *ps_list;

int main()
{
	setup((t_messlist **)&match_class, (method)match_new, (method)match_free, 
		(short)sizeof(t_match), 0L, A_GIMME, 0);
	addint((method)match_int);
	addfloat((method)match_float);
	addmess((method)match_list,		"list",		A_GIMME, 0);
	addmess((method)match_anything,	"anything",	A_GIMME, 0);
	addmess((method)match_set,		"set",		A_GIMME,0);					
	addmess((method)match_clear,	"clear", 	0);
	addmess((method)match_assist,	"assist",A_CANT,0);
	finder_addclass("Control","match");	
	ps_nn = gensym("nn");
	ps_list = gensym("list");

	return 0;
}

void match_assist(t_match *x, void *b, long m, long a, char *s)
{
	if (m == ASSIST_INLET)
		sprintf(s,"Input Sequence to be Matched");
	else 
		sprintf(s,"list of Matched Sequence");
}

void match_freebytes(t_match *x)
{
	if (x->m_seen)
		freebytes(x->m_seen,(short)(x->m_size * sizeof(t_atom)));
	if (x->m_want)
		freebytes(x->m_want,(short)(x->m_size * sizeof(t_atom)));
}

void match_int(t_match *x, long n)
{
	t_atom a;
	
	a.a_type = A_LONG;
	a.a_w.w_long = n;
	if (x->m_size)
		match_atom(x,&a);
}

void match_float(t_match *x, double f)
{
	t_atom a;
	
	a.a_type = A_FLOAT;
	a.a_w.w_float = f;
	if (x->m_size)
		match_atom(x,&a);
}

void match_list(t_match *x, t_symbol *s, short argc, t_atom *argv)
{
	long i;
	
	if (!x->m_size)
		return;
		
	for (i = 0; i < argc; i++)
		match_atom(x,argv+i);
}

void match_anything(t_match *x, t_symbol *s, short argc, t_atom *argv)
{
	t_atom a;
	
	if (!x->m_size)
		return;

	a.a_type = A_SYM;
	a.a_w.w_sym = s;
	match_atom(x,&a);
	match_list(x,s,argc,argv);
}

void match_atom(t_match *x, t_atom *a)
{
	long i;
	
	for (i = 0; i < x->m_size-1; i++)
		x->m_seen[i] = x->m_seen[i+1];
	x->m_seen[x->m_size-1] = *a;
	atom_compare(x);
}

void atom_compare(t_match *x)
{
	long i;
	
	for (i = 0; i < x->m_size; i++) {
		if (!atom_equal(x->m_seen+i,x->m_want+i))
			return;
	}
	outlet_atomlist(x->m_out,x->m_size,x->m_seen);
}

Boolean atom_equal(t_atom *a, t_atom *b)
{
	// check wild card match
	if (b->a_type == A_SYM && b->a_w.w_sym == ps_nn)
		return true;
		
	// matching type
	if (a->a_type == b->a_type) {
		if (a->a_w.w_long == b->a_w.w_long)
			return true;
		else
			return false;
	} else if (a->a_type == A_FLOAT && b->a_type == A_LONG) {	// different types
		float temp = a->a_w.w_float;
		long itemp = (long)temp;
		if (temp == (float)itemp) {
			if (itemp == b->a_w.w_long)
				return true;
		}
		return false;
	} else if (b->a_type == A_FLOAT && a->a_type == A_LONG) {
		float temp = b->a_w.w_float;
		long itemp = (long)temp;
		if (temp == (float)itemp) {
			if (itemp == a->a_w.w_long)
				return true;
		}
		return false;
	} else
		return false;
}

void outlet_atomlist(void *out, long argc, t_atom *argv)
{
	if (argc == 1) {
		if (argv->a_type == A_LONG)
			outlet_int(out,argv->a_w.w_long);
		else if (argv->a_type == A_FLOAT)
			outlet_float(out,argv->a_w.w_float);
		else if (argv->a_type == A_SYM)
			outlet_anything(out,argv->a_w.w_sym,0,0);
	} else if (argv->a_type == A_FLOAT || argv->a_type == A_LONG)
		outlet_list(out,ps_list,argc,argv);
	else if (argv->a_type == A_SYM)
		outlet_anything(out,argv->a_w.w_sym,argc-1,argv+1);
}

void match_clear(t_match *x)
{
	long i;
	
	for (i = 0; i < x->m_size; i++)
		x->m_seen[i] = atom_novalue;
}

void match_set(t_match *x, t_symbol *s, short ac, t_atom *av)
{
	t_atom *temp;
	char savelock;

	if (!ac)
		return;

	if (ac != x->m_size)
		temp = (t_atom *)getbytes((long)ac * sizeof(t_atom));
	else
		temp = x->m_want;
	match_setwant(temp,ac,av);
	savelock = lockout_set(1);
	critical_enter(x->m_critical);
	if (ac != x->m_size) {
		match_freebytes(x);
		x->m_want = temp;
		x->m_seen = (t_atom *)getbytes((long)ac * sizeof(t_atom));
	}
	x->m_size = ac;
	match_clear(x);
	lockout_set(savelock);
	critical_exit(x->m_critical);
}	
	
void match_setwant(t_atom *temp, short ac, t_atom *av)
{
	long i;
	
	for (i = 0; i < ac; i++)
		temp[i] = av[i];
}

void match_free(t_match *x)
{
	match_freebytes(x);
	critical_free(x->m_critical);
}

void *match_new(t_symbol *s, short ac, t_atom *av)
{
	t_match *x;
	
	x = (t_match *)newobject(match_class);
	x->m_out = outlet_new((t_object *)x,0);
	critical_new(&x->m_critical);
	if (ac) {
		x->m_want = (t_atom *)getbytes((long)ac * sizeof(t_atom));
		match_setwant(x->m_want,ac,av);
		x->m_seen = (t_atom *)getbytes((long)ac * sizeof(t_atom));
		x->m_size = ac;
		match_clear(x);
	} else {
		x->m_seen = 0;
		x->m_want = 0;
		x->m_size = 0;
	}
	return x;
}			
