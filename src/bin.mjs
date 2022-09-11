#!/usr/bin/env node

import path from 'node:path';
import { spawnSync } from "node:child_process";
import * as url from 'url';

const __dirname = url.fileURLToPath(new URL('.', import.meta.url));

const parentDir = path.join(__dirname, "..")

const nodeLoaderPath = path.join(parentDir, "rescript-ssg/src/node-loader.mjs");

spawnSync(
  "node",
  [`--experimental-loader=${nodeLoaderPath}`, ...process.argv.slice(2)],
  {
    stdio: "inherit",
    shell: true,
  }
);
