# --*-- coding: utf-8 --*--
from datetime import datetime
from py.xml import html
import pytest

def pytest_html_report_title(report):
    report.title = "arduino-k210 测试报告"

def pytest_html_results_table_header(cells):
    cells.insert(2, html.th("Description"))
    cells.insert(1, html.th("Time", class_="sortable time", col="time"))
    cells.pop()

def pytest_html_results_table_row(report, cells):
    cells.insert(2, html.td(report.description))
    cells.insert(1, html.td(datetime.utcnow(), class_="col-time"))
    cells.pop()

@pytest.hookimpl(hookwrapper=True)
def pytest_runtest_makereport(item, call):
    outcome = yield
    report = outcome.get_result()
    report.description = str(item.function.__doc__)

pytest_plugins = ("pytest_embedded.plugin",)
