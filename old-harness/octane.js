// cd v8/running/
// ../v8/out/x64.release/d8 --initial-heap-size=142 --max-heap-size=142 --no-lazy --predictable --predictable-gc-schedule ./bin/octane.js -- ../octane box2d 3 true
//
// ./v8/out/x64.release-header-markbit/d8 --harness --harness_lib=./evaluation/harness/harness.so --initial-heap-size=142 --max-heap-size=142 --no-lazy --predictable --predictable-gc-schedule ./evaluation/harness/octane.js -- ./octane box2d 30 true

if (globalThis.harnessPrepare) harnessPrepare();

const octane_dir = arguments[0];
const benchmark_name = arguments[1];
const iterations = arguments[2] || 1;
const gc_before_harness = arguments[3] == "true";

const base_dir = octane_dir + '/';

load(base_dir + 'base.js');

switch (benchmark_name) {
  case "box2d": load(base_dir + 'box2d.js'); break;
  case "codeload": load(base_dir + 'code-load.js'); break;
  case "crypto": load(base_dir + 'crypto.js'); break;
  case "deltablue": load(base_dir + 'deltablue.js'); break;
  case "earleyboyer": load(base_dir + 'earley-boyer.js'); break;
  case "gameboy": load(base_dir + 'gbemu-part1.js'); load(base_dir + 'gbemu-part2.js'); break;
  case "mandreel": load(base_dir + 'mandreel.js'); break;
  case "richards": load(base_dir + 'richards.js'); break;
  case "raytrace": load(base_dir + 'raytrace.js'); break;
  case "regexp": load(base_dir + 'regexp.js'); break;
  case "splay": load(base_dir + 'splay.js'); break;
  case "navierstokes": load(base_dir + 'navier-stokes.js'); break;
  case "pdfjs": load(base_dir + 'pdfjs.js'); break;
  case "zlib": load(base_dir + 'zlib.js'); load(base_dir + 'zlib-data.js'); break;
  case "typescript": load(base_dir + 'typescript.js'); load(base_dir + 'typescript-input.js'); load(base_dir + 'typescript-compiler.js'); break;
  default: throw new Error(`Unknown benchmark '${benchmark_name}'`);
}

var success = true;

function PrintResult(name, result) {
  print(name + ': ' + result);
}

function PrintError(name, error) {
  PrintResult(name, error);
  success = false;
}

function PrintScore(score) {
  if (success) {
    print('----');
    print('Score (version ' + BenchmarkSuite.version + '): ' + score);
  }
}

function RunOneIteration(iter, isWarmup) {
  if (isWarmup) print(`===== DaCapo ${benchmark_name} starting warmup =====`);
  else print(`===== DaCapo ${benchmark_name} starting =====`);

  if (!isWarmup && globalThis.harnessBegin) {
    if (gc_before_harness) {
        v8GC();
        print(`GC before harness completed ...`);
    }
    harnessBegin();
  }
  const startTime = new Date();
  // for (let i = 0; i < 10; i++) {
  const { time, score, latencyScore } = BenchmarkSuite.RunOnce();
  // }
  const endTime = new Date();
  const delta = endTime.getTime() - startTime.getTime();

  if (success) {
    if (isWarmup) print(`===== DaCapo ${benchmark_name} completed warmup in ${delta} msec =====`);
    else print(`===== DaCapo ${benchmark_name} PASSED in ${delta} msec =====`);
  } else {
    print(`===== DaCapo ${benchmark_name} FAILED ${isWarmup ? "warmup" : "code: -1"} in ${delta} msec =====`);
  }

  if (!isWarmup && globalThis.harnessEnd) harnessEnd();
  // if (!isWarmup) {
  //   // Print results
  //   print('============================ MMTk Statistics Totals ============================');
  //   print('GC	score	score.latency');
  //   print(`0	${score}	${latencyScore}	`);
  //   print(`Total time: ${time} ms`);
  //   print('------------------------------ End MMTk Statistics -----------------------------');
  // }
}

function assert(b, msg = "''") {
  if (!b) throw new Error('Assertion Failed: ' + msg);
}

BenchmarkSuite.config.doWarmup = false;
BenchmarkSuite.config.doDeterministic = true;

BenchmarkSuite.prototype.Setup = function (runner) {
  BenchmarkSuite.ResetRNG();
  this.results = [];
  this.runner = runner;
  for (const benchmark of this.benchmarks) {
    benchmark.Setup();
  }
}

BenchmarkSuite.prototype.TearDown = function () {
  for (const benchmark of this.benchmarks) {
    benchmark.TearDown();
  }
}

BenchmarkSuite.prototype.RunStep = function () {
  BenchmarkSuite.ResetRNG();
  let data, startTime, delta;
  try {
    startTime = new Date();
    for (const benchmark of this.benchmarks) {
      do {
        data = this.RunSingleBenchmark(benchmark, data);
      } while (data != null);
    }
    const endTime = new Date();
    delta = endTime.getTime() - startTime.getTime();
  } catch (e) {
    const endTime = new Date();
    delta = endTime.getTime() - startTime.getTime();
    this.NotifyError(e);
  }
  return delta;
}

BenchmarkSuite.RunOnce = function () {
  BenchmarkSuite.scores = [];
  // Assuming we only run one benchmark here
  function RunStep() {
    const suite = BenchmarkSuite.suites[0];
    suite.results = [];
    const time = suite.RunStep(suite.runner);
    // Calculate score
    const mean = BenchmarkSuite.GeometricMeanTime(suite.results);
    const score = BenchmarkSuite.FormatScore(100 * (suite.reference[0] / mean));
    // Calculate latency score
    let latencyScore = NaN;
    if (suite.reference.length == 2) {
      const meanLatency = BenchmarkSuite.GeometricMeanLatency(suite.results);
      if (meanLatency != 0) {
        latencyScore = BenchmarkSuite.FormatScore(100 * (suite.reference[1] / meanLatency));
      }
    }
    suite.NotifyResult();
    return { time, score, latencyScore };
  }
  return RunStep();
}

assert(BenchmarkSuite.suites.length == 1);
const suite = BenchmarkSuite.suites[0];
suite.Setup({
  NotifyResult: PrintResult,
  NotifyError: PrintError,
  NotifyScore: PrintScore
});
for (let i = 0; i < iterations; i++) {
  const isWarmup = i != iterations - 1;
  RunOneIteration(i, isWarmup);
}
suite.TearDown();
