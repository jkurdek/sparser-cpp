import os
import pandas as pd


def convert_parquet_to_json(directory: str, filenames: list, output_file: str) -> None:
    """
    Convert a list of Parquet files to a single JSON file.

    :param directory: Directory containing the Parquet files.
    :param filenames: List of Parquet files to be combined.
    :param output_file: Output JSON file.
    """
    combined_df = pd.concat(
        [pd.read_parquet(os.path.join(directory, filename)) for filename in filenames],
        ignore_index=True,
    )

    combined_df.to_json(output_file, orient="records", lines=True)
    print(f"Conversion to JSON completed successfully! Output saved to: {output_file}")


if __name__ == "__main__":
    directory = "samples/wikipedia/"
    filenames = ["a.parquet", "b.parquet", "c.parquet", "d.parquet"]
    output_file = "samples/wikipedia.json"

    convert_parquet_to_json(directory, filenames, output_file)
