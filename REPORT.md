# Fit Strategy Comparison — Results

All numbers below come directly from `test_patterns.c` (see `results.csv` for
the raw data). Each pattern runs 300 live pointer slots through ~20,000
allocation operations (except `burst`, which allocates once and frees half),
then a fragmentation snapshot is taken **before** final cleanup — that
snapshot is what's reported here.

`fragmentation_ratio = 1 - (largest_free_block / total_free_bytes)`
Close to 0 means free memory is basically one big usable chunk. Close to 1
means it's shattered into many small, mostly-useless pieces.

## Results

| Pattern | Strategy | Free bytes | Largest free block | # free blocks | Arenas grown | Frag. ratio |
|---|---|---:|---:|---:|---:|---:|
| random | first_fit | 14,464 | 3,232 | 74 | 1 | 0.777 |
| random | best_fit | 16,080 | 7,952 | 49 | 1 | **0.506** |
| random | worst_fit | 185,504 | 560 | 930 | **4** | 0.997 |
| uniform | first_fit | 50,176 | 48,512 | 12 | 1 | 0.033 |
| uniform | best_fit | 50,336 | 49,088 | 7 | 1 | **0.025** |
| uniform | worst_fit | 37,280 | 160 | 415 | 1 | 0.996 |
| increasing | first_fit | 291,952 | 5,824 | 350 | 7 | 0.980 |
| increasing | best_fit | 178,272 | 55,584 | 158 | **5** | **0.688** |
| increasing | worst_fit | 853,120 | 2,080 | 1,093 | 16 | 0.998 |
| burst | first_fit | 80,832 | 42,224 | 152 | 2 | 0.478 |
| burst | best_fit | 80,864 | 42,544 | 151 | 2 | 0.474 |
| burst | worst_fit | 80,832 | 42,224 | 152 | 2 | 0.478 |

## Findings

**Worst-fit is worst, by a wide margin.** Across every pattern with mixed
sizes, worst-fit produces the highest fragmentation ratio (0.996–0.998) and
the most arena growth — 4x more memory requested from the OS than first-fit
or best-fit on the `random` pattern, and 16 arenas (over 1 MB) on
`increasing` versus 5 for best-fit. Deliberately taking the *largest*
available chunk for every request keeps carving up your biggest blocks into
many small unusable leftovers. This matches the standard textbook result.

**Best-fit consistently beats first-fit** on fragmentation ratio when sizes
vary (`random`: 0.506 vs 0.777; `increasing`: 0.688 vs 0.980), and needed
fewer arena growths on `increasing` (5 vs 7). This is because best-fit
actively hunts for the tightest-fitting hole instead of taking the first one
that's big enough, so it leaves fewer awkward mid-size gaps behind. The
tradeoff (not shown in this data, but true of the algorithm) is that
best-fit is O(n) per allocation in the worst case — it must scan the whole
free list to be sure it found the *best* fit, whereas first-fit can stop
early.

**On uniform-size workloads, the fit strategy barely matters** — first-fit
and best-fit both land near-zero fragmentation (0.033 / 0.025), because
every freed hole is exactly the size of the next request regardless of which
one gets picked. Worst-fit still manages to fragment badly here (0.996) by
consistently choosing the one oversized leftover chunk and slicing it
smaller and smaller each time. **Takeaway: the size distribution of the
workload matters more than the strategy — when requests are all the same
size, strategy choice is nearly irrelevant.**

**The `burst` pattern shows identical results for all three strategies.**
This is expected: during the initial allocation burst, the free list has at
most one entry (or none), so there's no real "choice" to make between
first/best/worst fit — they all pick the same block because there's only
one option. Strategy differences only show up once there's a genuinely
mixed population of free blocks to choose among, which is exactly what the
`random` and `increasing` patterns create by interleaving frees with
allocations of varying sizes.

## Recommendation

For a general-purpose allocator handling unpredictable, mixed-size
workloads, **best-fit is the safer default** of the three implemented here —
it consistently produced the lowest fragmentation ratio and the least
arena growth in every mixed-size test. First-fit is a reasonable choice
when allocation speed matters more than memory efficiency, since it doesn't
need to scan the full free list. Worst-fit should be avoided; nothing in
this data set gives it an advantage over the other two.

## Threading note

`test_thread.c` runs 8 threads × 5,000 alloc/free operations each,
concurrently, against the same shared allocator instance, with per-byte
canary verification. All runs pass with no corruption — the global mutex
added around the free-list operations (see `mymalloc.c`) is sufficient for
correctness, though it does serialize all allocator calls across threads
(a known scalability limit; sharding by arena or using per-thread arenas
would be the next step if throughput under contention mattered).
