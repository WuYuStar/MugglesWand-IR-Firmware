"""Extract the firmware's actual embedded HTML for local inspection (no device control)."""
from pathlib import Path
import argparse

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('output', type=Path, help='Destination HTML file')
args = parser.parse_args()
source = Path(__file__).resolve().parents[1] / 'src/Service/WebPage.h'
page = source.read_text().split('R"raw(\n', 1)[1].rsplit(')raw";', 1)[0]
args.output.parent.mkdir(parents=True, exist_ok=True)
args.output.write_text(page)
print(args.output.resolve())
