#ifndef ____stats__
#define ____stats__

struct global_stats{
	int lock;

	int total_vul;
	int total_evil;
	int total_spam;

	int total_packets;
	int time_start;
	int last_print;
	int bytes_handled;
};

/*Updates all statistics according to that packet*/
void update_stats(volatile struct global_stats* stats, struct packet_info *current_packet, int code);

//prints global stats
void simple_stats_print(volatile struct global_stats* stats);

#endif

