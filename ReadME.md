# Application 2: Engineering Analysis - Preemptive Scheduling

### Student: Andre Llanos
### UCFID: 4809793

## Implementation notes
The light sensor PCB  does not like being plugged in backwards and got a bit too hot. So most of the testing was with Wokwi for this application.


## 1. Task Timing and Jitter

looking at the logs it seems that the sensor task is very consistant as it uses the vtaskdelayuntil while the LED and print uses vtaskdelay `vTaskDelay` specifies a **relative delay** from the moment it is called. The actual period of these tasks is `(task execution time) + (delay time)`. Because the Sensor has priority, it creates a jitter in the other tasks as when there is an overlap time, the sensor task will run, delaying the other 


## 2. Priority-Based Preemption

I was not able to catch this in the simulation but An example where this can be an issue would be around -  
1.  **Tick 999:** The `print_status_task` (priority 0) wakes from its 1000ms delay and begins executing its `printf` function.
2.  **Tick 1000:** A timer interrupt fires, and the FreeRTOS scheduler sees that the `sensor_task` (priority 2) is now ready to run (its 200ms delay from tick 800 has expired).
3.  **Immediate Preemption:** Because the `sensor_task` has a higher priority than the `print_status_task`, the scheduler immediately saves the context of the print task (pausing it mid-execution) and switches to the sensor task.
4.  **Tick 1000-1015:** The `sensor_task` runs, reads the ADC, performs its calculations, and calls `vTaskDelayUntil`. It is now in the Blocked state, waiting until tick 1200.
5.  **Resumption:** The scheduler now sees that the highest-priority ready task is the `print_status_task` (priority 0). It restores its context, and the `printf` function completes its execution.

The sensor task does not wait for the print task to finish; it interrupts it immediately. This ensures that time-critical security monitoring is never delayed by less important background logging.

## 3. Effect of Task Execution Time

if the sensor task has a longer execution time than its period then at the end of the task it will try and run again and assuming that it still cannot complete its task in time, it will take over all processing of the core effectively starving it.
Some ways to fix this would be to 1. make the code more efficient so that it uses less time 2. Allow it to run on another core so that this does not affect another task. 



## 4. `vTaskDelay` vs `vTaskDelayUntil`

We chose `vTaskDelayUntil` for the sensor task because sensor sampling is a **hard real-time** requirement in this context; it must happen at precise, regular intervals to be effective.

* `vTaskDelay(period)`: This function puts a task to sleep for a *relative* duration. The actual period becomes `period + execution_time`. If the execution time varies (due to other interrupts or complex calculations), the sampling rate will drift, and small errors will accumulate over time, making the data unreliable.

* `vTaskDelayUntil(&lastWakeTime, period)`: This function unblocks a task at an *absolute* tick count. It maintains a fixed frequency by calculating the next wake time based on the *previous* wake time, not when the delay call was made. This automatically corrects for jitter caused by the task's own execution time, solving the problem of cumulative drift.

For the LED blink task, a small amount of drift is inconsequential. Its purpose is a simple visual indication for a human observer, which is a **soft real-time** requirement. Whether it blinks every 1000 ms or 1015 ms makes no functional difference, so the simpler `vTaskDelay` is perfectly acceptable.

## 5. Thematic Integration Reflection (Hardware Security)

In our Hardware Security system, the tasks map directly to critical functions:
* **`print_status_task` (Low Priority):** This represents a routine, non-critical **system logger**. It periodically records that the system is functioning normally. It's useful for diagnostics but can be delayed without compromising security.
* **`led_task` (Medium Priority):** This is the **"System OK" heartbeat indicator**. It provides a quick visual confirmation that the device is powered on and the scheduler is running. It's more important than a log entry but less critical than an active threat sensor.
* **`sensor_task` (High Priority):** this is the main function of the program so must be reliable **intrusion detection system**. It uses a light sensor to detect if the secure device's enclosure has been opened. 

## Bonus: Starvation Experiment

To demonstrate starvation, the `sensor_task` was modified to never block. The `vTaskDelayUntil()` call at the end of its `while(1)` loop was replaced with an empty infinite loop: `while(1) {}`.

**Code Change:**
```c
// In sensor_task, replace vTaskDelayUntil(...) with:
while(1) {
    // This high-priority task now never yields the CPU.
}

```


## AI use
- Used in this readme to make my writing more legible ( its pretty good at formatting to highlight points and structuring to be easier to read)
- I Used it recommend scenarios where unexpectedscheduling issues will occur 
- I had some issues with the sensor task and troubleshot that with AI
