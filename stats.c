#include "kernel.h"

/*Updates all statistics according to that packet*/
void update_stats(volatile struct global_stats* stats, struct packet_info *current_packet, int code){
	spin_lock(&stats->lock);

    printf("Updating stats.");

    stats->total_packets++;
    stats->bytes_handled += current_packet->packet_length;

    if(code & 1) stats->total_spam++;
    if(code & 2) stats->total_vul++;
    if(code & 4) stats->total_evil++;

    unlock(&stats->lock);
}

void simple_stats_print(volatile struct global_stats* stats){
	spin_lock(&stats->lock);

    int cycle = current_cpu_cycles();
    int secs1 = ((double) (cycle - stats->time_start))/((double) CPU_CYCLES_PER_SECOND);
    //int secs2=(cycle-last_print)/CPU_CYCLES_PER_SECOND;
    //last_print=cycle;
    int r1 = stats->total_packets/secs1;
    int r2 = stats->bytes_handled/secs1;
    //int r3=total_packets/(secs2+1);
    //int r4=bytes_handled/(secs2+1);
    printf("----------totals---------\n");
    printf("Totals: %d packets %d bytes\n", stats->total_packets, stats->bytes_handled );
    printf("Speed: ~%d packets/sec ~%d bytes/sec\n", r1, r2 );
    //printf("Since Last Print: ~%d packets/sec ~%d bytes/sec\n", r3, r4 );
    printf("Evil %d Vulnerable %d Spam %d\n", stats->total_evil, stats->total_vul, stats->total_spam);
    printf("-------------------------\n");

    unlock(&stats->lock);
}
