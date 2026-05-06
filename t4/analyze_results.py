#!/usr/bin/env python3
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import sys

df = pd.read_csv('result.txt', sep='\s+')

plt.figure(figsize=(12, 5))

plt.subplot(1, 2, 1)
for delay in sorted(df['delay_ms'].unique()):
    subset = df[df['delay_ms'] == delay]
    plt.plot(subset['clients'], subset['overhead_sec'], 'o-', label=f'{delay}ms')
plt.xlabel('Number of clients')
plt.ylabel('Overhead (sec)')
plt.title('Overhead vs Clients')
plt.legend()
plt.grid(alpha=0.3)

df_delayed = df[df['delay_ms'] > 0].copy()

plt.subplot(1, 2, 2)
plt.scatter(df_delayed['max_client_delay_sec'], df_delayed['server_time_sec'],
            c=df_delayed['delay_ms'], cmap='viridis', s=50, alpha=0.7)
plt.plot([0, df['max_client_delay_sec'].max()],
         [0, df['max_client_delay_sec'].max()], 'r--', label='ideal: server_time = client_delay')
plt.xlabel('Max client delay (sec)')
plt.ylabel('Server time (sec)')
plt.title('Server time vs Slowest client')
plt.legend()
plt.grid(alpha=0.3)

plt.tight_layout()
plt.savefig('analysis.png', dpi=150, bbox_inches='tight')