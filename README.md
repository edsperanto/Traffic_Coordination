# coord_traffic
A traffic simulator created for CPSC 3500 by Edward Gao.

### How to compile

```
make clean
make
```

### How to run
```
./coord_traffic <number_of_cars>
```

### Threads
- Four threads
    - Thread 1: `north_traffic()`, spawns north-bound traffic
    - Thread 2: `south_traffic()`, spawns south-bound traffic
    - Thread 3: `flag_person()`, coordinate traffic and spawns passing_car() threads
    - Thread 4: `passing_car()`, simulate a car passing through a lane.

### Semaphores
- One semaphore: `car_queues_count`
    - Initial value: 0
    - Allow traffic lanes to signal car arrivals to flag person
    - Signals when new cars arrive and when there's no traffic

### Mutex Locks
- Two mutex locks
    - `lane_mutex`: Enforce only one car passing through lane at any time.
    - `car_queue_mutex`: Make sure no new cars arrive when flag person is letting a car through.

### Strengths
- Concurrency in execution allows for a more faithful representation of real traffic flow. 

### Weaknesses
- Limited Functionality, lack of parameter customization by user.
- Inflexible simulation, specifically for one traffic situation.