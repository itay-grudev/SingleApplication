const fs = require('fs');
const path = require('path');
const { spawn } = require('child_process');

const workspace = process.env.GITHUB_WORKSPACE || process.cwd();
const isWindows = process.platform === 'win32';

function log(message) {
  console.log(`> ${message}`);
}

function sleep(ms) {
  return new Promise((resolve) => setTimeout(resolve, ms));
}

function assert(condition, message) {
  if (!condition) {
    throw new Error(message);
  }
}

function collectFiles(rootDir, fileName) {
  const matches = [];

  function walk(current) {
    const entries = fs.readdirSync(current, { withFileTypes: true });
    for (const entry of entries) {
      const fullPath = path.join(current, entry.name);
      if (entry.isDirectory()) {
        if (entry.name === 'CMakeFiles') {
          continue;
        }
        walk(fullPath);
        continue;
      }
      if (entry.isFile() && entry.name === fileName) {
        matches.push(fullPath);
      }
    }
  }

  walk(rootDir);
  return matches;
}

function findExecutable(exampleName) {
  const exampleRoot = path.join(workspace, 'examples', exampleName);
  const expectedName = isWindows ? `${exampleName}.exe` : exampleName;
  const defaultPath = path.join(exampleRoot, expectedName);

  if (fs.existsSync(defaultPath)) {
    return defaultPath;
  }

  const candidates = collectFiles(exampleRoot, expectedName);
  if (candidates.length === 0) {
    throw new Error(`Unable to find executable for ${exampleName} under ${exampleRoot}`);
  }

  candidates.sort((a, b) => a.length - b.length);
  return candidates[0];
}

function spawnManaged(command, args = []) {
  const child = spawn(command, args, {
    cwd: workspace,
    stdio: ['ignore', 'pipe', 'pipe']
  });

  let stdout = '';
  let stderr = '';
  let finished = false;
  let exitCode = null;

  child.stdout.setEncoding('utf8');
  child.stderr.setEncoding('utf8');

  child.stdout.on('data', (chunk) => {
    stdout += chunk;
  });

  child.stderr.on('data', (chunk) => {
    stderr += chunk;
  });

  child.on('close', (code) => {
    finished = true;
    exitCode = code;
  });

  return {
    child,
    getOutput() {
      return `${stdout}${stderr}`;
    },
    isFinished() {
      return finished;
    },
    getExitCode() {
      return exitCode;
    }
  };
}

async function runAndWait(command, args, timeoutMs = 10000) {
  const proc = spawnManaged(command, args);
  const start = Date.now();

  while (!proc.isFinished()) {
    if (Date.now() - start > timeoutMs) {
      try {
        proc.child.kill();
      } catch (err) {
        // Best effort timeout cleanup.
      }
      throw new Error(`Process timed out: ${command} ${args.join(' ')}`);
    }
    await sleep(50);
  }

  return {
    exitCode: proc.getExitCode(),
    output: proc.getOutput()
  };
}

async function waitForOutput(proc, predicate, timeoutMs, context) {
  const start = Date.now();
  while (Date.now() - start < timeoutMs) {
    const output = proc.getOutput();
    if (predicate(output)) {
      return output;
    }
    if (proc.isFinished()) {
      throw new Error(`${context} ended early with code ${proc.getExitCode()}. Output:\n${output}`);
    }
    await sleep(100);
  }
  throw new Error(`Timed out waiting for ${context}. Output:\n${proc.getOutput()}`);
}

function killProcess(proc, label) {
  if (!proc || !proc.child || proc.isFinished()) {
    return;
  }

  log(`Cleaning up ${label}`);
  try {
    proc.child.kill();
  } catch (err) {
    // Best effort cleanup.
  }
}

async function main() {
  const basicExe = findExecutable('basic');
  const sendingExe = findExecutable('sending_arguments');

  log(`Using basic executable: ${basicExe}`);
  log(`Using sending_arguments executable: ${sendingExe}`);

  let basicPrimary;
  let sendingPrimary;

  try {
    log('Verifying basic example rejects a second instance');
    basicPrimary = spawnManaged(basicExe, []);

    await waitForOutput(
      basicPrimary,
      (out) => out.includes('Started a new instance'),
      5000,
      'basic primary instance startup'
    );

    log('Launching basic secondary instance');
    const basicSecondary = await runAndWait(basicExe, [], 5000);

    assert(
      basicSecondary.exitCode === 0,
      `Secondary basic instance exit code was ${basicSecondary.exitCode}, expected 0. Output:\n${basicSecondary.output}`
    );

    assert(
      !basicSecondary.output.includes('Started a new instance'),
      `Secondary basic instance should not print startup output. Output:\n${basicSecondary.output}`
    );

    assert(
      !basicPrimary.isFinished(),
      `Primary basic instance terminated unexpectedly with code ${basicPrimary.getExitCode()}. Output:\n${basicPrimary.getOutput()}`
    );

    killProcess(basicPrimary, 'basic primary process');
    basicPrimary = null;

    const token = `ci-token-${Date.now()}-${Math.floor(Math.random() * 1000000)}`;
    log('Verifying sending_arguments forwards a message to the primary instance');
    sendingPrimary = spawnManaged(sendingExe, []);

    await sleep(500);
    assert(
      !sendingPrimary.isFinished(),
      `sending_arguments primary exited unexpectedly with code ${sendingPrimary.getExitCode()}. Output:\n${sendingPrimary.getOutput()}`
    );

    log(`Launching sending_arguments secondary instance with token: ${token}`);
    const sendingSecondary = await runAndWait(sendingExe, [token], 7000);

    assert(
      sendingSecondary.exitCode === 0,
      `Secondary sending_arguments instance exit code was ${sendingSecondary.exitCode}, expected 0. Output:\n${sendingSecondary.output}`
    );

    assert(
      sendingSecondary.output.includes('App already running.'),
      `Secondary sending_arguments instance did not report forwarding mode. Output:\n${sendingSecondary.output}`
    );

    await waitForOutput(
      sendingPrimary,
      (out) => out.includes(token),
      7000,
      'forwarded message on sending_arguments primary'
    );

    log('Node integration checks completed successfully');
  } finally {
    killProcess(basicPrimary, 'basic primary process');
    killProcess(sendingPrimary, 'sending_arguments primary process');
  }
}

main().catch((error) => {
  console.error(error.message || error);
  process.exit(1);
});
