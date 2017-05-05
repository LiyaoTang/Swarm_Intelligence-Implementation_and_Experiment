#include <iostream>
#include <ctime>
using namespace std;

// Product State
class State
{
public:
	int cur_process;
	int waited_time;

	State() :cur_process(0), waited_time(0) {}
};

class Particle
{
public:
	int* cur_pos;
	int evaluation;
	int* pbest_ind;
	int pbest_ind_evaluation;

	Particle();
	void evaluate();
	void update_pbest();
	void evolve();

	static int product_num;
	static int machine_num;
	static int* pbest_group;
	static int pbest_group_evaluation;

	static double rand_mutation;
	static double rand_history_impact;
	static double rand_group_impact;
};
int Particle::product_num = 0;
int Particle::machine_num = 0;
int* Particle::pbest_group = NULL;
int Particle::pbest_group_evaluation = INT_MAX;
/// ----------- CONSTANT SETTING ------------ ///
double Particle::rand_mutation = 0.9;
double Particle::rand_history_impact = 0.1;
double Particle::rand_group_impact = 0.1;

class Process
{
public:
	int mach_num;
	int time;
	Process(int n = 0, int t = 0) :mach_num(n), time(t) {}

	friend void Particle::evaluate();

	static Process** table;
};
Process** Process::table = NULL;

class Candidate
{
public:
	int can;
	Candidate *next;
	Candidate(int c = 0, Candidate *n = NULL) :can(c), next(n) {}
};
enum Pattern { Rand, FIFO, SJF, WSJF, LPT, EDF, Pattern_num };
class Machine
{
public:
	int num;
	Pattern pattern;
	Candidate *candidates;
	Candidate *tail;
	unsigned int can_num;
	unsigned int time_needed;
	int cur_pro;

	Machine(Pattern p = Pattern_num, int cn = 0) :num(0), pattern(p), can_num(cn), time_needed(0), cur_pro(0)
	{
		candidates = new Candidate();
		tail = candidates;
	}
	void add_candidate(int can)
	{
		tail->next = new Candidate(can);
		tail = tail->next;
		can_num++;
	}
	void pick_candidates(State* state);
	~Machine()
	{
		Candidate *p = candidates->next;
		while (p != NULL)
		{
			delete candidates;
			candidates = p;
			p = p->next;
		}
		delete candidates;
	}
	void print()
	{
		cout << "pattern: " << pattern << endl;
		cout << "cur_pro: " << cur_pro << endl;
		cout << "can_num: " << can_num << endl;
		Candidate* p = candidates;
		while (p->next != NULL)
		{
			cout << p->next->can << " ";
			p = p->next;
		}
		cout << endl;
	}
};

void Machine::pick_candidates(State* state)
{
	if (can_num == 0)
	{
		cur_pro = 0;
		return;
	}

	int rnt = 0;
	Candidate *aim = candidates;
	switch (this->pattern)
	{
	case Rand:
	{
		int cnt = rand() % can_num; // Guaranteed : temp->next will never be NULL
		for (int i = 0; i < cnt; i++) aim = aim->next;
		rnt = aim->next->can;

		// ------ //
		Candidate *temp = aim->next->next;
		if (tail == aim->next) tail = aim;
		delete aim->next;
		aim->next = temp;

		can_num--;
		time_needed = Process::table[rnt][state[rnt].cur_process].time;
		cur_pro = rnt;
		break;
	}
	case FIFO:
	{
		rnt = aim->next->can;

		// ------ //
		Candidate *temp = aim->next->next;
		if (tail == aim->next) tail = aim;
		delete aim->next;
		aim->next = temp;

		can_num--;
		time_needed = Process::table[rnt][state[rnt].cur_process].time;
		cur_pro = rnt;
		break;
	}
	case SJF:
	{
		int cur_can = aim->next->can;
		int min = Process::table[cur_can][state[cur_can].cur_process].time;
		Candidate *p = candidates;
		while (p->next != NULL)
		{
			cur_can = p->next->can;
			int cur_time = Process::table[cur_can][state[cur_can].cur_process].time;
			if (min > cur_time)
			{
				min = cur_time;
				aim = p;
			}

			p = p->next;
		}
		rnt = aim->next->can;

		// ------ //
		Candidate *temp = aim->next->next;
		if (tail == aim->next) tail = aim;
		delete aim->next;
		aim->next = temp;

		can_num--;
		time_needed = Process::table[rnt][state[rnt].cur_process].time;
		cur_pro = rnt;
		break;
	}
	case WSJF:
	{
		int cur_can = aim->next->can;
		int min = Process::table[cur_can][state[cur_can].cur_process].time;
		double min_weight = min * (1 - (double)state[cur_can].waited_time / min);

		Candidate *p = candidates;
		while (p->next != NULL)
		{
			cur_can = p->next->can;
			int cur_time = Process::table[cur_can][state[cur_can].cur_process].time;
			double cur_weight = cur_time * (1 - (double)state[cur_can].waited_time / cur_time);
			if (min_weight > cur_weight)
			{
				min_weight = cur_weight;
				min = cur_time;
				aim = p;
			}

			p = p->next;
		}
		rnt = aim->next->can;

		p = candidates->next;
		while (p != NULL)
		{
			state[p->can].waited_time += min;
			p = p->next;
		}
		state[rnt].waited_time = 0;

		// ------ //
		Candidate *temp = aim->next->next;
		if (tail == aim->next) tail = aim;
		delete aim->next;
		aim->next = temp;

		can_num--;
		time_needed = Process::table[rnt][state[rnt].cur_process].time;
		cur_pro = rnt;
		break;
	}
	case LPT:
	{
		int cur_can = aim->next->can;
		int max = Process::table[cur_can][state[cur_can].cur_process].time;
		Candidate  *p = candidates;
		while (p->next != NULL)
		{
			cur_can = p->next->can;
			int cur_time = Process::table[cur_can][state[cur_can].cur_process].time;
			if (max < cur_time)
			{
				max = cur_time;
				aim = p;
			}

			p = p->next;
		}

		rnt = aim->next->can;

		// ------ //
		Candidate *temp = aim->next->next;
		if (tail == aim->next) tail = aim;
		delete aim->next;
		aim->next = temp;

		can_num--;
		time_needed = Process::table[rnt][state[rnt].cur_process].time;
		cur_pro = rnt;
		break;
	}
	case EDF:
	{
		int pro_num = Particle::product_num;
		int mach_num = Particle::machine_num;

		int min_end = INT_MAX;
		for (int i = 1; i <= pro_num; i++)
		{
			int cur_end = 0;
			int cur_state = state[i].cur_process;
			Process *cur_seq = Process::table[i];

			for (int j = 0; j < mach_num; j++)
			{
				if (j < cur_state)
				{
					if (cur_seq[j].mach_num != this->num)continue;
					else break;
				}
				else if (cur_seq[j].mach_num != this->num) cur_end += cur_seq[j].time;
				else
				{
					cur_end += cur_seq[j].time;
					break;
				}
			}

			if (cur_end != 0 && cur_end < min_end)
			{
				min_end = cur_end;
				rnt = i;
			}
		}

		Candidate *p = candidates;
		aim = NULL;
		while (p->next != NULL)
		{
			if (p->next->can == rnt) aim = p;
			p = p->next;
		}

		// ------ //
		if (aim == NULL)
		{
			cur_pro = 0;
			time_needed = 0;
			break;
		}
		Candidate *temp = aim->next->next;
		if (tail == aim->next) tail = aim;
		delete aim->next;
		aim->next = temp;

		can_num--;
		time_needed = Process::table[rnt][state[rnt].cur_process].time;
		cur_pro = rnt;
		break;
	}
	default:
	{
		this->print();
		cout << "State: " << endl;
		for (int i = 1; i <= Particle::product_num; i++)
		{
			cout << "Pro :" << i << " cur_process: " << state[i].cur_process << endl;
		}
		cout << "aim->next->can :" << aim->next->can << endl;
	}
	}
}

Particle::Particle()
{
	cur_pos = new int[machine_num + 1];
	evaluation = 0;
	pbest_ind = new int[machine_num + 1];
	pbest_ind_evaluation = INT_MAX;

	// random initialize
	for (int i = 1; i <= machine_num; i++)
	{
		int r = rand() % Pattern_num;
		cur_pos[i] = r;
		pbest_ind[i] = r;
	}

	this->evaluate();
	this->update_pbest();
}

void Particle::evaluate()
{
	State *state = new State[product_num + 1];		// product state
	Machine *machine = new Machine[machine_num + 1];	// machines : working pattern 

	for (int i = 1; i <= product_num; i++)
	{
		machine[Process::table[i][0].mach_num].add_candidate(i);
	}
	for (int i = 1; i <= machine_num; i++)
	{
		machine[i].num = i;
		machine[i].pattern = (Pattern)cur_pos[i];
		machine[i].pick_candidates(state);
	}

	// ---- compute ---- //
	int clock = 0;
	while (1)
	{
		int min_needed = INT_MAX;
		for (int i = 1; i <= machine_num; i++)
		{
			if (machine[i].time_needed > 0 && min_needed > machine[i].time_needed) min_needed = machine[i].time_needed;
		}
		clock += min_needed;

		for (int i = 1; i <= machine_num; i++)
		{
			if (machine[i].cur_pro == 0) continue;

			machine[i].time_needed -= min_needed;
			if (machine[i].time_needed == 0)
			{
				int cur_pro = machine[i].cur_pro;
				int next_process = ++state[cur_pro].cur_process;
				if (cur_pro == 0 || next_process >= machine_num) continue;

				int next_mach = Process::table[cur_pro][next_process].mach_num;
				machine[next_mach].add_candidate(cur_pro);
			}
		}

		int done = 1;
		for (int i = 1; i <= machine_num; i++)
		{
			if (machine[i].time_needed == 0) machine[i].pick_candidates(state);
			if (machine[i].cur_pro != 0) done = 0;
		}

		if (done) break;
	}

	evaluation = clock;

	delete[] state;
	delete[] machine;
}

void Particle::update_pbest()
{
	// ---- update pbest (ind / group) ---- //
	if (pbest_ind_evaluation > evaluation)
	{
		pbest_ind_evaluation = evaluation;
		for (int i = 1; i <= machine_num; i++)
		{
			pbest_ind[i] = cur_pos[i];
		}

		// group
		if (pbest_group_evaluation > evaluation)
		{
			pbest_group_evaluation = evaluation;
			for (int i = 1; i <= machine_num; i++)
			{
				pbest_group[i] = cur_pos[i];
			}
		}
	}
}

void Particle::evolve()
{
	double m = (double)(rand() % 1000) / 1000.0;
	if (m < rand_mutation)
	{
		int a = (rand() % (machine_num)) + 1;
		int b = (rand() % (machine_num)) + 1;
		if (a > b) { int temp = a; a = b; b = temp; }

		for (int i = a; i <= b; i++)
		{
			cur_pos[i] = rand() % Pattern_num;
		}
	}

	m = (double)(rand() % 1000) / 1000.0;
	if (m < rand_history_impact)
	{
		int a = (rand() % machine_num) + 1;
		int b = (rand() % machine_num) + 1;
		if (a > b) { int temp = a; a = b; b = temp; }

		for (int i = a; i <= b; i++)
		{
			cur_pos[i] = pbest_ind[i];
		}
	}

	m = (double)(rand() % 1000) / 1000.0;
	if (m < rand_history_impact)
	{
		int a = (rand() % machine_num) + 1;
		int b = (rand() % machine_num) + 1;
		if (a > b) { int temp = a; a = b; b = temp; }

		for (int i = a; i <= b; i++)
		{
			cur_pos[i] = pbest_group[i];
		}
	}
}

int main()
{
	freopen("1.txt", "r", stdin);
	srand((int)time(0));
	int start_t = time(0);
	cout << "Please enter num of product" << endl;
	int product_num = 0;
	cin >> product_num;
	cout << "Please enter num of machine" << endl;
	int machine_num = 0;
	cin >> machine_num;

	Particle::machine_num = machine_num;
	Particle::product_num = product_num;
	Particle::pbest_group = new int[machine_num + 1];

	cout << "Please enter time table" << endl;
	Process::table = new Process*[product_num + 1];
	for (int i = 1; i <= product_num; i++)
	{
		Process::table[i] = new Process[machine_num];
		for (int j = 0; j < machine_num; j++)
		{
			cin >> Process::table[i][j].time;
		}
	}
	cout << "Please enter sequence table" << endl;
	for (int i = 1; i <= product_num; i++)
	{
		for (int j = 0; j < machine_num; j++)
		{
			cin >> Process::table[i][j].mach_num;
		}
	}

	// ------- INPUT ---------- //

	Particle* particles = new Particle[product_num*machine_num];

	// Set iteration correspoundingly
	int iteration = product_num*machine_num * 10;
	for (int i = 0; i < iteration; i++)
	{
		for (int j = 0; j < product_num*machine_num; j++)
		{
			particles[j].evolve();
			particles[j].evaluate();
			particles[j].update_pbest();
		}

		// Ä£ÄâÍË»ðË¼Ïë
		Particle::rand_mutation = 0.9 - 0.4*i*i / (iteration*iteration);
		Particle::rand_history_impact = 0.1 + 0.5*i*i / (iteration*iteration);
		Particle::rand_group_impact = 0.1 + 0.5*i*i / (iteration*iteration);
	}

	cout << "Least time needed: " << Particle::pbest_group_evaluation << endl;
	cout << "Correspounding sequence:" << endl;
	for (int i = 1; i <= machine_num; i++)
	{
		cout << Particle::pbest_group[i] << " ";
	}
	cout << endl;
	system("pause");
}