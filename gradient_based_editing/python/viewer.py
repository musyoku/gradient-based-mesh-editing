import os, sys, argparse
import math
import numpy as np
import gradient_based_editing as gm


def main():
    viewer = gm.viewer.Viewer()
    viewer.go()


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    args = parser.parse_args()
    main()