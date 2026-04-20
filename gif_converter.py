from moviepy import VideoFileClip

directory = "saved_data/Test8/"
name = "field_lines_animation"

# Load the MP4 file
clip = VideoFileClip(f"{directory}{name}.mp4")

# Optional: Trim the clip to a specific segment (e.g., first 3 seconds)
# clip = clip.subclip(0, 3)

# Convert and save as GIF
clip.write_gif(f"{directory}{name}.gif", fps=5)
