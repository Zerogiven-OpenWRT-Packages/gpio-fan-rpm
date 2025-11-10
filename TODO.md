
  🟢 COULD IMPROVE (Nice to Have - Performance/Maintainability)

  1. Performance: Reduce clock_gettime() calls - Called every loop iteration in gpio.c:196-200 and 220-224. Check time every N iterations or use timerfd.
  2. JSON object creation overhead - In watch mode, repeatedly creating/destroying JSON objects. Consider reusing or caching format strings.
  3. UCI config error handling - args.c doesn't handle UCI read failures gracefully. Add fallback logic.
