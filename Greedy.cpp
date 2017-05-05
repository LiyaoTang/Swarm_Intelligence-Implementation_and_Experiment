#include <iostream>
using namespace std;

#define brink 5.0 // ãÐÖµ

class Process
{
public:
	int mach_num;
	int time;
	Process(int mn = 0, int t = 0) :mach_num(mn), time(t) {}

	static Process** table;
};
Process** Process::table = NULL;

class Machine;
class Candidate
{
public:
	int can;
	
	double weight;
	Candidate *next;
	
	Candidate(int c = 0, int s = 0, Candidate *n = NULL) :can(c), next(n) {}
	void update_weight(Machine* machines, int* state, int* wait);
};

class Machine
{
public:
	int can_num;
	int time_needed;
	Candidate *candidates;
	int cur;

	static int machine_num;

	Machine(int c_num = 0) : can_num(0), time_needed(0)
	{
		candidates = new Candidate();
		cur = 0;
	}

	void add_candidate(int can, int state)
	{
		Candidate* temp = candidates->next;
		candidates->next = new Candidate(can, state, temp);
		this->can_num++;
	}
	
	void pick_candidate(int *state, int* wait)
	{
		if (can_num == 0)
		{
			time_needed = 0;
			cur = 0;
			return;
		}

		Candidate *p = candidates;
		Candidate *min = p;
		int min_weight = INT_MAX;
		while (p->next != NULL)
		{
			if (p->next->weight < min_weight)
			{
				min_weight = p->next->weight;
				min = p;
			}
			p = p->next;
		}

		cur = min->next->can;
		int cur_state = state[cur];
		time_needed = Process::table[cur][cur_state].time;
		
		can_num--;
		p = candidates;
		while (p->next != NULL)
		{
			wait[p->next->can] += time_needed;
			p = p->next;
		}
		wait[cur] = 0;

		Candidate *temp = min->next->next;
		delete min->next;
		min->next = temp;
	}
	
	void update_can(Machine *machines, int* state, int* wait)
	{
		Candidate *p = candidates;
		while (p->next != NULL)
		{
			p->next->update_weight(machines, state, wait);
			p = p->next;
		}
	}
};
int Machine::machine_num = 0;

void Candidate::update_weight(Machine* machines, int* state, int* wait)
{
	int cur_state = state[can];
	int cur_t = Process::table[can][cur_state].time;
	weight =  cur_t - wait[can]/cur_t;
	int next_state = cur_state + 1;
	if (next_state < Machine::machine_num)// have to work on another mach later
	{
		int next_mach = Process::table[can][next_state].mach_num;
		weight -= brink * (1 - (double(machines[next_mach].can_num) / Machine::machine_num));
	}
}

int main()
{
	freopen("1.txt", "r", stdin);
	cout << "Please enter num of Candidate" << endl;
	int Product_num = 0;
	cin >> Product_num;

	cout << "Please enter num of machine" << endl;
	int machine_num = 0;
	cin >> machine_num;
	Machine::machine_num = machine_num;

	Process::table = new Process*[Product_num + 1];
	cout << "Please enter time table" << endl;
	for (int i = 1; i <= Product_num; i++)
	{
		Process::table[i] = new Process[machine_num];
		for (int j = 0; j < machine_num; j++)
		{
			cin >> Process::table[i][j].time;
		}
	}
	cout << "Please enter sequence table" << endl;
	for (int i = 1; i <= Product_num; i++)
	{
		for (int j = 0; j < machine_num; j++)
		{
			cin >> Process::table[i][j].mach_num;
		}
	}

	int *state = new int[Product_num + 1];
	int *wait = new int[Product_num + 1];
	for (int i = 1; i <= Product_num; i++)
	{
		state[i] = 0;
		wait[i] = 0;
	}
	// initialize Machine	
	Machine *machines = new Machine[machine_num + 1];
	for (int i = 1; i <= Product_num; i++)
	{
		machines[Process::table[i][0].mach_num].add_candidate(i, 0);
	}
	for (int i = 1; i <= machine_num; i++)
	{
		machines[i].update_can(machines, state, wait);
		machines[i].pick_candidate(state, wait);
	}
	// -------------- INITIALIZATION FINISHED ----------------//
	int clock = 0;
	while (1)
	{
		int min_t = INT_MAX;
		for (int i = 1; i <= machine_num; i++)
		{
			if(machines[i].time_needed == 0) machines[i].pick_candidate(state, wait);
			if (machines[i].time_needed != 0 && machines[i].time_needed < min_t)
			{
				min_t = machines[i].time_needed;
			}
		}
		for (int i = 1; i <= machine_num; i++)
		{
			if(machines[i].time_needed != 0) machines[i].time_needed -= min_t;
		}
		clock += min_t;

		int over = 1;
		for (int i = 1; i <= machine_num; i++)
		{
			if (machines[i].time_needed == 0 && machines[i].cur != 0)
			{
				int can = machines[i].cur;
				int cur_state = ++state[can];
				
				if (cur_state < machine_num)
				{
					int next_mach = Process::table[can][cur_state].mach_num;
					machines[next_mach].add_candidate(can, cur_state);
				}
			}
			if (machines[i].time_needed != 0 || machines[i].can_num != 0) over = 0;
		}

		if (over) break;
	}

	cout << "We think the least time it will take is "<< clock << endl;

	system("pause");
	return 0;
}