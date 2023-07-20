import numpy as np
from typing import Generator


def crop_cube(cube: np.ndarray) -> np.ndarray:
    """
    Crops an np.array to have no all-zero padding around the edge.

    Given in https://stackoverflow.com/questions/39465812/how-to-crop-zero-edges-of-a-numpy-array

    Parameters:
    cube (np.array): 3D Numpy byte array where 1 values indicate polycube positions

    Returns:
    np.array: Cropped 3D Numpy byte array equivalent to cube, but with no zero padding

    """
    nonzero_indices = np.where(cube != 0)
    cropped_cube = cube[np.min(nonzero_indices[0]):np.max(nonzero_indices[0]) + 1,
                         np.min(nonzero_indices[1]):np.max(nonzero_indices[1]) + 1,
                         np.min(nonzero_indices[2]):np.max(nonzero_indices[2]) + 1]
    return cropped_cube


def expand_cube(cube: np.ndarray) -> Generator[np.ndarray, None, None]:
    """
    Expands a polycube by adding single blocks at all valid locations.

    Calculates all valid new positions of a polycube by shifting the existing cube +1 and -1 in each dimension.
    New valid cubes are returned via a generator function, in case they are not all needed.

    Parameters:
    cube (np.array): 3D Numpy byte array where 1 values indicate polycube positions

    Returns:
    generator(np.array): Yields new polycubes that are extensions of cube

    """
    cube = np.pad(cube, 1, 'constant', constant_values=0)
    output_cube = np.array(cube)

    xs, ys, zs = cube.nonzero()
    output_cube[xs+1, ys, zs] = 1
    output_cube[xs-1, ys, zs] = 1
    output_cube[xs, ys+1, zs] = 1
    output_cube[xs, ys-1, zs] = 1
    output_cube[xs, ys, zs+1] = 1
    output_cube[xs, ys, zs-1] = 1

    exp = (output_cube ^ cube).nonzero()

    for (x, y, z) in zip(exp[0], exp[1], exp[2]):
        new_cube = np.array(cube)
        new_cube[x, y, z] = 1
        yield crop_cube(new_cube)
