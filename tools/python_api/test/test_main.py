import pytest

from test_datatype import *
from test_parameter import *
from test_exception import *
from test_df import *
from test_write_to_csv import *

if __name__ == "__main__":
    raise SystemExit(pytest.main([__file__]))
