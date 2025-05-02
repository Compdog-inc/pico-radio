#include <lwip/stats.h>
#include <lwip/memp.h>
#include <lwip/mem.h>
#include <lwip/tcp.h>
#include <lwip/priv/tcp_priv.h>

#include "lwipdebug.h"

void LWIP::PrintLwipTcpPcbStatus()
{
    printf("==== LWIP TCP PCB Status ====\n");

    // Active connections
    struct tcp_pcb *pcb = tcp_active_pcbs;
    int active_count = 0;
    while (pcb != nullptr)
    {
        printf("  [ACTIVE] local %u -> remote %u, state %d\n",
               pcb->local_port, pcb->remote_port, pcb->state);
        pcb = pcb->next;
        active_count++;
    }
    printf("Total ACTIVE PCBs: %d\n", active_count);

    // Listening sockets
#if TCP_LISTEN_BACKLOG
    struct tcp_pcb_listen *listen_pcb = tcp_listen_pcbs.listen_pcbs;
    int listen_count = 0;
    while (listen_pcb != nullptr)
    {
        printf("  [LISTEN] port %u, backlog %d\n",
               listen_pcb->local_port, listen_pcb->accepts_pending);
        listen_pcb = listen_pcb->next;
        listen_count++;
    }
    printf("Total LISTEN PCBs: %d\n", listen_count);
#endif

    // TIME-WAIT connections
    pcb = tcp_tw_pcbs;
    int timewait_count = 0;
    while (pcb != nullptr)
    {
        printf("  [TIME_WAIT] local %u -> remote %u\n",
               pcb->local_port, pcb->remote_port);
        pcb = pcb->next;
        timewait_count++;
    }
    printf("Total TIME_WAIT PCBs: %d\n", timewait_count);

#if LWIP_STATS && MEMP_STATS
    printf("\n==== LWIP MEMP Stats ====\n");
    printf("TCP PCBs used: %d / %d\n",
           lwip_stats.memp[MEMP_TCP_PCB]->used,
           lwip_stats.memp[MEMP_TCP_PCB]->avail);
#endif

#if LWIP_STATS_DISPLAY
    printf("\n==== Full LWIP Stats Dump ====\n");
    stats_display(); // Dump all memory and protocol stats
#endif

    printf("\n==== PCB Accounting Summary ====\n");
    int total_used = active_count + listen_count + timewait_count;
    printf("Total PCBs in use (active + listen + time_wait): %d\n", total_used);
}