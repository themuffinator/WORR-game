struct {
	int		score;
	int		thawed;
	float	win_time;
	bool	update;
	float	last_update;
	int		frozen;
	int		alive;
	float	break_time;
	bool	ready;
} freeze[5];

typedef struct hnode {
	struct pnode*	entries;
	int				cur;
	int				num;
} hndnode;

typedef struct pnode {
	char*	text;
	int	align;
	void*	arg;
	void	(*selectfunc)(gentity_t* ent, struct pnode* entry);
} pmenunode;

int	endMapIndex;

void gibThink(gentity_t* ent);
