This praxis illustrates the usage of TIMERS in TCV/VNETI. Having written
../HOOKS, I realized that TCV hooks (TCV_HOOKS) are not really needed there,
as we never get into a situation when a hooked packet is deallocated without
the plugin's explicit knowledge. So this is a version without TCV_HOOKS, but
with a somewhat improved reception.
