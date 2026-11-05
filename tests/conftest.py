# --*-- coding: utf-8 --*--
from datetime import datetime, timezone
from html import escape

import pytest


def pytest_html_report_title(report):
    report.title = 'arduino-k210 测试报告'


def pytest_html_results_table_header(cells):
    cells.insert(2, '<th>Description</th>')
    cells.insert(1, '<th class="sortable time" data-column-type="time">Time</th>')
    cells.pop()


def pytest_html_results_table_row(report, cells):
    cells.insert(2, f'<td>{escape(getattr(report, "description", ""))}</td>')
    cells.insert(1, f'<td class="col-time">{datetime.now(timezone.utc).isoformat()}</td>')
    cells.pop()


@pytest.hookimpl(hookwrapper=True)
def pytest_runtest_makereport(item, call):
    outcome = yield
    report = outcome.get_result()
    report.description = str(item.function.__doc__)


pytest_plugins = ('pytest_embedded.plugin',)
