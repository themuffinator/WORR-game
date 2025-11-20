import sys
import unittest

import tools.ci.run_tests as run_tests


class _DummyProcess:
    def __init__(self):
        self.terminated = False
        self.killed = False

    def terminate(self):
        self.terminated = True

    def kill(self):
        self.killed = True


class SignalHandlingTests(unittest.TestCase):
    def setUp(self) -> None:
        run_tests.SHUTDOWN_EVENT.clear()
        run_tests.ACTIVE_PROCESSES.clear()

    def test_request_shutdown_marks_event_and_terminates_active_processes(self):
        dummy = _DummyProcess()
        run_tests.ACTIVE_PROCESSES.add(dummy)
        run_tests.request_shutdown()
        self.assertTrue(run_tests.shutdown_requested())
        self.assertTrue(dummy.terminated or dummy.killed)
        self.assertNotIn(dummy, run_tests.ACTIVE_PROCESSES)

    def test_run_subprocess_respects_shutdown_requests(self):
        run_tests.request_shutdown()
        with self.assertRaises(run_tests.ShutdownRequested):
            run_tests.run_subprocess([sys.executable, "-c", "print('hi')"], run_tests.REPO_ROOT)

    def test_run_subprocess_tracks_active_processes(self):
        completed = run_tests.run_subprocess(
            [sys.executable, "-c", "print('ok')"],
            run_tests.REPO_ROOT,
        )
        self.assertEqual(completed.returncode, 0)
        self.assertIn("ok", completed.stdout)
        self.assertEqual(run_tests.ACTIVE_PROCESSES, set())


if __name__ == "__main__":
    unittest.main()
