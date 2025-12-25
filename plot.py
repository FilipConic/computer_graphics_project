#! /usr/bin/python

import matplotlib.pyplot as plt

out_points = [(435, -180), (435, -85), (325, -85), (186, -33), (137, 111), (137, 450), (183, 597), (311, 650), (428, 610), (468, 498), (468, 427), (466, 427), (430, 474), (362, 491), (266, 448), (231, 330), (231, 223), (276, 100), (400, 55), (525, 101), (572, 228), (572, 498), (541, 624), (452, 709), (315, 740), (164, 705), (67, 605), (33, 450), (33, 111), (68, -44), (169, -144), (325, -180)]
in_points = [(349, 160), (332, 223), (332, 330), (349, 392), (400, 415), (450, 393), (468, 330), (468, 223), (450, 160), (400, 138)]
points = [(398, -180), (402, 99), (384, 61), (373, 45), (360, 31), (346, 19), (329, 8), (311, 0), (292, -5), (250, -10), (218, -7), (190, 0), (164, 10), (140, 26), (120, 47), (102, 71), (88, 99), (78, 129), (72, 163), (72, 386), (78, 420), (88, 451), (102, 478), (120, 503), (141, 523), (164, 539), (190, 550), (219, 557), (250, 560), (293, 555), (312, 549), (330, 541), (347, 531), (374, 504), (385, 487), (394, 469), (401, 450), (401, 550), (523, 550), (523, -180)]

# (295, 334), (399, 506), (426, 550)
# (295 - 299, 334 - 506) x (426 - 299, 550 - 506)
# (-4, -172) x (127, 44)
# -4 * 44 - (-172) * 127

x = [t[0] for t in points]
y = [t[1] for t in points]

labels = [str(i) for i in range(len(points))]

fig, axs = plt.subplots(1, 2, figsize = (12, 6))
axs[0].scatter(x, y, c='blue', alpha=0.6)

for i, label in enumerate(labels):
    axs[0].annotate(label, (x[i], y[i]), 
                xytext=(5, 5),  # Offset label by 5 points right and up
                textcoords='offset points',
                fontsize=10)

axs[0].grid(True, alpha=0.3)

x = [t[0] for t in out_points]
y = [t[1] for t in out_points]
labels = [str(i) for i in range(len(out_points))]
axs[1].scatter(x, y, c='blue', alpha=0.6)

for i, label in enumerate(labels):
    axs[1].annotate(label, (x[i], y[i]), 
                xytext=(5, 5),  # Offset label by 5 points right and up
                textcoords='offset points',
                fontsize=10)

x = [t[0] for t in in_points]
y = [t[1] for t in in_points]
labels = [str(i) for i in range(len(in_points))]
axs[1].scatter(x, y, c='red', alpha=0.6)

for i, label in enumerate(labels):
    axs[1].annotate(label, (x[i], y[i]), 
                xytext=(5, 5),  # Offset label by 5 points right and up
                textcoords='offset points',
                fontsize=10)
axs[1].grid(True, alpha=0.3)

plt.show()
