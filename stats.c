#include "kernel.h"

/*Updates all statistics according to that packet*/
void update_stats(volatile struct global_stats* stats, struct packet_info *current_packet, int code){
	spin_lock(&stats->lock);

    stats->total_packets++;
    stats->bytes_handled += current_packet->packet_length;

    //printf("On update: total packets = %d, total bytes = %d.\n", stats->total_packets, stats->bytes_handled);

    if(code & 1) stats->total_spam++;
    if(code & 2) stats->total_vul++;
    if(code & 4) stats->total_evil++;

    unlock(&stats->lock);
}

void simple_stats_print(volatile struct global_stats* stats){
	spin_lock(&stats->lock);

    int cycle = current_cpu_cycles();
    //printf("Cycles: %d.\n", cycle - stats->time_start);
    int secs1 = ((int) (cycle - stats->time_start))/((int) CPU_CYCLES_PER_SECOND);
    //printf("Seconds: %d.\n", secs1);
    //int secs2=(cycle-last_print)/CPU_CYCLES_PER_SECOND;
    //last_print=cycle;
    if(secs1==0)secs1=1;
    int r1 = stats->total_packets/secs1;
    int r2 = stats->bytes_handled/secs1;

    printf("----------totals---------\n");
    printf("Totals: %d packets %d bytes\n", stats->total_packets, stats->bytes_handled );
    printf("Speed: ~%d packets/sec ~%d bytes/sec\n", r1, r2 );
    //printf("Since Last Print: ~%d packets/sec ~%d bytes/sec\n", r3, r4 );
    printf("Evil %d Vulnerable %d Spam %d\n", stats->total_evil, stats->total_vul, stats->total_spam);
    printf("-------------------------\n");

    unlock(&stats->lock);
}
