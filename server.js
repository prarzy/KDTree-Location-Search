const express = require('express');
const { execFile } = require('child_process');
const app = express();
const port = 3000;

app.use(express.static(__dirname));

// API endpoint to handle search requests
app.get('/search', (req, res) => {
    const { lat, lon, category, radius } = req.query;
    if (!lat || !lon || !category) return res.status(400).send("Missing parameters");

    const args = [lat, lon, category];
    if (radius) args.push(radius); // optional radius for "within range"

    execFile('./search.exe', args, (err, stdout, stderr) => {
        if (err) return res.status(500).send(stderr);
        try {
            const data = JSON.parse(stdout);
            res.json(data);
        } catch (e) {
            res.status(500).send("Error parsing JSON from C program");
        }
    });
});

app.listen(port, () => console.log(`Server running at http://localhost:${port}`));

