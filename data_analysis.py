import json
from pathlib import Path

import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from matplotlib.patches import Ellipse


DATA_PATH = Path("output/data.json")
FPS = 5


def load_sim_data(path: Path):
    with path.open("r") as f:
        j = json.load(f)

    width = int(j["width"])
    height = int(j["height"])

    potential = np.asarray(j["potential"], dtype=float)
    potential = potential.reshape(len(potential), height, width)

    moving_circles = j["moving_circles"]
    static_circles = j["static_circles"]

    return {
        "width": width,
        "height": height,
        "potential": potential,
        "moving_circles": moving_circles,
        "static_circles": static_circles,
    }


def animate_circles(data, output_path="circles_animation.mp4"):
    width = data["width"]
    height = data["height"]
    moving = data["moving_circles"]
    static = data["static_circles"]

    n_frames = min(len(moving), len(static))

    fig, ax = plt.subplots(figsize=(8, 6))
    ax.set_xlim(0, width)
    ax.set_ylim(0, height)
    ax.set_aspect("equal")
    ax.set_title("Circle Positions vs Time")
    ax.set_xlabel("x")
    ax.set_ylabel("y")

    # Optional: match simulation coordinate feel
    # ax.invert_yaxis()

    patches = []
    title = ax.text(
        0.02, 0.98, "", transform=ax.transAxes,
        ha="left", va="top"
    )

    def clear_patches():
        nonlocal patches
        for p in patches:
            p.remove()
        patches = []

    def draw_frame(frame_idx):
        clear_patches()

        for c in moving[frame_idx]:
            e = Ellipse(
                xy=(c["x"], c["y"]),
                width=2 * c["a"],
                height=2 * c["b"],
                fill=False,
                linewidth=1.5,
            )
            ax.add_patch(e)
            patches.append(e)

        for c in static[frame_idx]:
            e = Ellipse(
                xy=(c["x"], c["y"]),
                width=2 * c["a"],
                height=2 * c["b"],
                fill=False,
                linewidth=2.0,
                linestyle="--",
            )
            ax.add_patch(e)
            patches.append(e)

        title.set_text(f"Frame {frame_idx}")
        return patches + [title]

    ani = animation.FuncAnimation(
        fig, draw_frame, frames=n_frames, interval=1000 // FPS, blit=False
    )

    writer = animation.FFMpegWriter(fps=FPS)
    ani.save(output_path, writer=writer)
    plt.close(fig)


def animate_potential(data, output_path="potential_animation.mp4"):
    potential = data["potential"]
    n_frames, height, width = potential.shape

    fig, ax = plt.subplots(figsize=(8, 6))
    ax.set_title("Potential Heat Map vs Time")
    ax.set_xlabel("x")
    ax.set_ylabel("y")

    vmin = np.min(potential)
    vmax = np.max(potential)

    im = ax.imshow(
        potential[0],
        origin="lower",
        aspect="auto",
        vmin=vmin,
        vmax=vmax,
    )
    cbar = fig.colorbar(im, ax=ax)
    cbar.set_label("Potential")

    title = ax.text(
        0.02, 0.98, "", transform=ax.transAxes,
        ha="left", va="top", color="white"
    )

    def update(frame_idx):
        im.set_data(potential[frame_idx])
        title.set_text(f"Frame {frame_idx}")
        return [im, title]

    ani = animation.FuncAnimation(
        fig, update, frames=n_frames, interval=1000 // FPS, blit=False
    )

    writer = animation.FFMpegWriter(fps=FPS)
    ani.save(output_path, writer=writer)
    plt.close(fig)


def main():
    data = load_sim_data(DATA_PATH)

    animate_circles(data, "output/circles_animation.mp4")
    animate_potential(data, "output/potential_animation.mp4")


if __name__ == "__main__":
    main()