# 03-twister

The ``example-application`` directory contains a custom board called ``unleashed`` (a renamed version of Zephyr's ``hifive_unleashed`` target) and two samples, ``hello_world`` and ``shell_module``, copied there for convenience.

## Running tests

To build the sample, initialize Zephyr with:

```
west init -l example_application
```

Then enter the ``example_application`` directory and run tests:

```
west twister --platform unleashed -T hello_world
```

or, for a more detailed output:

```
west twister -vv --platform unleashed -T hello_world
```

The Robot-based test can be run with:

```
west twister --platform stm32f7 -T shell_module
```

To see more details on the test results, open ``twister-out/stm32f7/sample.shell.shell_module.robot/report.html`` in your browser.

The Ztest-based test (unit tests of the shell's dynamic commands, see below) runs on the same board in Renode:

```
west twister --platform stm32f7 -T ztest_module
```

## Enabling Renode support in boards

To enable Renode support for the ``unleashed`` target, the entries below have been added.

In ``unleashed.yaml``:

```
simulation: renode
simulation_exec: renode
```

In ``board.cmake``:

```
set(SUPPORTED_EMU_PLATFORMS renode)
set(RENODE_SCRIPT ${CMAKE_CURRENT_LIST_DIR}/support/hifive_unleashed.resc)
set(RENODE_UART sysbus.uart0)
```

Also, the ``support/hifive_unleashed.resc`` file was added to describe the Renode configuration for this board.

An analogous process for the ``stm32f7`` target was also performed, with only minor differences.

## Enabling Robot test in samples

The ``shell_module`` sample has a Robot test enabled.

This required a change to the sample's ``sample.yaml`` file:

```
  sample.shell.shell_module.robot:
    harness: robot
    harness_config:
      robot_test_path: shell_module.robot
```

The ``shell_module.robot`` file needs to include the following section:

```
*** Settings ***
Resource                      ${KEYWORDS}
```

With the resource defined as above, you can use the ``Prepare Machine`` keyword to set up the platform based on your Twister target and load the appropriate ELF file.

## Enabling Ztest tests

The ``ztest_module`` directory contains a Ztest suite that checks the ``dynamic`` shell commands (``add``, ``execute``, ``remove``, ``show``) implemented in ``shell_module/src/dynamic_cmd.c``. It is built for the same ``stm32f7`` board and executed in Renode by Twister, like the Robot tests of ``shell_module``.

```
ztest_module/
|-- CMakeLists.txt          # compiles the tests and ../shell_module/src/dynamic_cmd.c
|-- prj.conf                # Ztest + shell with the "dummy" backend (no UART)
|-- testcase.yaml           # Twister entry: harness ztest, platform stm32f7
`-- src/test_dynamic_cmd.c  # the 10 test cases
```

Twister needs the following in ``testcase.yaml``:

```
tests:
  shell.ztest.dynamic_cmd:
    harness: ztest
    platform_allow: stm32f7
```

With ``harness: ztest`` and the board's ``simulation: renode``, Twister starts the ELF in Renode (using the board's ``.resc`` file, the same one the Robot tests use), reads the UART output and marks the test as passed when Ztest prints ``PROJECT EXECUTION SUCCESSFUL``. No ``.robot`` file is needed.

``CMakeLists.txt`` compiles the original ``dynamic_cmd.c`` from ``shell_module`` (it is not copied), so the Ztest and Robot tests exercise exactly the same source:

```
target_sources(app PRIVATE
  src/test_dynamic_cmd.c
  ../shell_module/src/dynamic_cmd.c
)
```

``prj.conf`` enables the shell with the dummy backend instead of the serial one. The tests call ``shell_execute_cmd()`` from inside the firmware and read the text the command printed with ``shell_backend_dummy_get_output()``:

```
CONFIG_ZTEST=y
CONFIG_ZTEST_NEW_API=y
CONFIG_SHELL=y
CONFIG_SHELL_BACKEND_SERIAL=n
CONFIG_SHELL_BACKEND_DUMMY=y
```

### How the Ztest and Robot tests complement each other

| | Robot (``shell_module/*.robot``) | Ztest (``ztest_module``) |
|---|---|---|
| Point of view | From outside: types on the UART, reads the console | From inside: calls the shell API in the firmware |
| Checks | Text shown to a user, timing, logs, bypass mode, UART re-init | Text **and** return codes (``0``, ``-ENOEXEC``, ``-EINVAL``) |
| Strength | Real UART path, interactive scenarios | Boundaries (20 commands, 32 characters), fast and precise failure messages |

The expected messages are the same in both (``command added successfully``, ``duplicated command``, ...). ``shell_module/shell_module_advanced.robot`` runs the dynamic commands through the UART; ``ztest_module`` covers the same commands with their limits.

Two things worth knowing when extending the suite:

- ``shell_backend_dummy_get_output()`` empties the backend buffer each time it is called, so the test helper reads it once per command and keeps a copy.
- The ``dynamic`` table is global to the firmware, so ``cleanup()`` empties it after each test. It removes the entries from the last one to the first because ``remove`` in ``dynamic_cmd.c`` moves one row too many with ``memmove()``: removing anything but the last entry of a full table (20 commands) reads 33 bytes past the end of the array.

## Regenerating platform description

The platform description file for ``stm32f7``, ``boards/arm/stm32f7/support/stm32f746g_disco.repl``, was generated using the ``dts2repl`` tool.

To rebuild the platform from a DTS file, run the following:

```
west build -p -b stm32f7 hello_world
dts2repl build/zephyr/zephyr.dts --output boards/arm/stm32f7/support/stm32f746g_disco.repl
```
